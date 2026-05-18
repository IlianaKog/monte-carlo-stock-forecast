/* global Chart */
'use strict';

let chart = null;

// ── Startup ──────────────────────────────────────────────────────────────────

async function loadCSVList() {
  const sel = document.getElementById('csv-select');
  try {
    const res = await fetch('/api/list-csv');
    const list = await res.json();
    sel.innerHTML = '';
    if (!list.length) {
      sel.innerHTML = '<option value="">No CSV files found in data/</option>';
      return;
    }
    for (const name of list.sort()) {
      const opt = document.createElement('option');
      opt.value = name;
      opt.textContent = name;
      sel.appendChild(opt);
    }
  } catch {
    sel.innerHTML = '<option value="">Server unreachable</option>';
  }
}

// ── Simulation ────────────────────────────────────────────────────────────────

document.getElementById('run-btn').addEventListener('click', runSimulation);

async function runSimulation() {
  const symbol = document.getElementById('csv-select').value;
  const days   = parseInt(document.getElementById('days-select').value);
  const sims   = parseInt(document.getElementById('sims-select').value);

  if (!symbol) return showError('Please select a symbol.');

  setLoading(true);
  hideError();

  try {
    // Step 1 — load historical prices from server
    const priceRes = await fetch(`/api/prices?symbol=${encodeURIComponent(symbol)}`);
    if (!priceRes.ok) {
      const err = await priceRes.json().catch(() => ({}));
      throw new Error(err.error || `HTTP ${priceRes.status}`);
    }
    const { prices } = await priceRes.json();

    // Step 2 — run Monte Carlo simulation
    const simRes = await fetch('/api/simulate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ symbol, days, simulations: sims, historical_prices: prices })
    });
    if (!simRes.ok) {
      const err = await simRes.json().catch(() => ({}));
      throw new Error(err.error || `HTTP ${simRes.status}`);
    }

    renderResults(await simRes.json());
  } catch (e) {
    showError(e.message);
  } finally {
    setLoading(false);
  }
}

// ── Render ────────────────────────────────────────────────────────────────────

function renderResults(d) {
  const fmt = n => '$' + n.toFixed(2);
  const pct = n => (n >= 0 ? '+' : '') + n.toFixed(2) + '%';
  const p   = d.statistics.percentiles;

  setText('stat-current', fmt(d.current_price));
  setText('stat-mean',    fmt(d.statistics.mean_final_price));
  setText('stat-std',     fmt(d.statistics.std_dev));
  setText('stat-var95',   fmt(d.statistics.var_95));
  setText('stat-var99',   fmt(d.statistics.var_99));
  setText('stat-p-low',   `${fmt(p.p5)} / ${fmt(p.p25)} / ${fmt(p.p50)}`);
  setText('stat-p-high',  `${fmt(p.p75)} / ${fmt(p.p95)}`);
  setText('param-mu',     d.parameters.mu.toFixed(6));
  setText('param-sigma',  d.parameters.sigma.toFixed(6));
  setText('param-sims',   d.simulations.toLocaleString('en-US'));

  const retEl = document.getElementById('stat-return');
  retEl.textContent = pct(d.statistics.expected_return_pct);
  retEl.className = d.statistics.expected_return_pct >= 0 ? 'positive' : 'negative';

  const probEl = document.getElementById('stat-prob');
  probEl.textContent = (d.statistics.probability_of_profit * 100).toFixed(1) + '%';
  probEl.className = d.statistics.probability_of_profit >= 0.5 ? 'positive' : 'negative';

  drawFanChart(d);
  document.getElementById('results').classList.remove('hidden');
}

// ── Fan chart ─────────────────────────────────────────────────────────────────
// Dataset order matters for fill: '+1' fills toward the NEXT dataset index.
// Order: p95 → p75 → mean → p25 → p5
// p95 fills down to p75 (outer top band), p75 fills down to mean (inner top),
// p25 fills down to p5 (inner bottom), p5 fills down to p25 (outer bottom).

function drawFanChart(d) {
  const labels = Array.from({ length: d.days + 1 }, (_, i) => i);
  const shared = { pointRadius: 0, tension: 0.3 };

  const datasets = [
    // outer top band: p95 → p75
    {
      ...shared,
      label: 'p95',
      data: d.paths.p95,
      borderColor: 'rgba(34,211,160,0.6)',
      borderWidth: 1,
      borderDash: [4, 3],
      backgroundColor: 'rgba(99,102,241,0.08)',
      fill: '+1'
    },
    // inner top band: p75 → mean
    {
      ...shared,
      label: 'p75',
      data: d.paths.p75,
      borderColor: 'rgba(99,102,241,0.4)',
      borderWidth: 1,
      backgroundColor: 'rgba(99,102,241,0.15)',
      fill: '+1'
    },
    // mean line
    {
      ...shared,
      label: 'Mean',
      data: d.paths.mean,
      borderColor: '#6366f1',
      borderWidth: 2.5,
      backgroundColor: 'transparent',
      fill: false
    },
    // inner bottom band: p25 → p5
    {
      ...shared,
      label: 'p25',
      data: d.paths.p25,
      borderColor: 'rgba(99,102,241,0.4)',
      borderWidth: 1,
      backgroundColor: 'rgba(99,102,241,0.15)',
      fill: '+1'
    },
    // outer bottom edge: p5
    {
      ...shared,
      label: 'p5',
      data: d.paths.p5,
      borderColor: 'rgba(248,113,113,0.6)',
      borderWidth: 1,
      borderDash: [4, 3],
      backgroundColor: 'rgba(99,102,241,0.08)',
      fill: false
    }
  ];

  if (chart) chart.destroy();
  chart = new Chart(document.getElementById('fan-chart'), {
    type: 'line',
    data: { labels, datasets },
    options: {
      animation: { duration: 500 },
      interaction: { mode: 'index', intersect: false },
      plugins: {
        legend: { labels: { color: '#8892a4', boxWidth: 12, padding: 16 } },
        tooltip: {
          callbacks: {
            label: ctx => `${ctx.dataset.label}: $${ctx.parsed.y.toFixed(2)}`
          }
        }
      },
      scales: {
        x: {
          ticks: { color: '#8892a4', maxTicksLimit: 10 },
          grid:  { color: '#2a2d3e' },
          title: { display: true, text: 'Trading Days', color: '#8892a4' }
        },
        y: {
          ticks: { color: '#8892a4', callback: v => '$' + v.toFixed(0) },
          grid:  { color: '#2a2d3e' },
          title: { display: true, text: 'Price (USD)', color: '#8892a4' }
        }
      }
    }
  });
}

// ── Helpers ───────────────────────────────────────────────────────────────────

function setText(id, text) { document.getElementById(id).textContent = text; }

function setLoading(on) {
  document.getElementById('spinner').classList.toggle('hidden', !on);
  document.getElementById('run-btn').disabled = on;
}

function showError(msg) {
  const box = document.getElementById('error-box');
  box.textContent = `Error: ${msg}`;
  box.classList.remove('hidden');
}

function hideError() {
  document.getElementById('error-box').classList.add('hidden');
}

// ── Init ──────────────────────────────────────────────────────────────────────
loadCSVList();

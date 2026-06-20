const ROUTES = [
  {
    key: "status",
    path: "/status",
    kicker: "Realtime monitoring",
    title: "Trang trạng thái robot",
    description: "Tổng quan sức khoe he thong, node ROS2 va cac luong dang chay."
  },
  {
    key: "monitor",
    path: "/monitor",
    kicker: "Mission control",
    title: "Monitor camera va Nav2 realtime",
    description: "Camera stream, map /map, duong di du kien, duong da di va dieu khien truc tiep."
  },
  {
    key: "slam",
    path: "/slam",
    kicker: "Mapping workflow",
    title: "Trang quet map SLAM",
    description: "Khoi dong SLAM, xem map truc tiep, luu map va quan ly ban do."
  },
  {
    key: "analyst",
    path: "/analyst",
    kicker: "Dataset insight",
    title: "Trang analyst",
    description: "Tong hop metric training, dataset run va chi so an toan / dieu khien."
  },
  {
    key: "dataset",
    path: "/dataset",
    kicker: "Collection ops",
    title: "Trang thu thap dataset",
    description: "Chay kich ban thu thap du lieu theo scenario, class va nhan."
  },
  {
    key: "logs",
    path: "/logs",
    kicker: "Operational trace",
    title: "Trang log",
    description: "Theo doi log runtime va danh sach node ROS2 dang hoat dong."
  }
];

const state = {
  route: getRouteFromLocation(),
  apiOnline: false,
  backendBase: getInitialBackendBase(),
  summary: null,
  telemetry: null,
  map: null,
  paths: { global_plan: [], local_plan: [], timestamp: null },
  datasetActive: null,
  datasetRuns: [],
  scenarios: [],
  savedMaps: [],
  nodes: [],
  logs: [],
  training: { running: false, history: [] },
  mapRasterCache: { key: null, canvas: null },
  monitorTrail: [],
  peerConnection: null,
  monitorStream: null,
  sockets: {
    telemetry: null,
    map: null,
    paths: null
  },
  refreshTimer: null,
  refreshInFlight: false,
  activeControl: null,
  pressedKeys: new Set()
};

const navEl = document.getElementById("nav");
const appEl = document.getElementById("app");
const pageKickerEl = document.getElementById("page-kicker");
const pageTitleEl = document.getElementById("page-title");
const apiStatusDotEl = document.getElementById("api-status-dot");
const apiStatusLabelEl = document.getElementById("api-status-label");
const refreshButtonEl = document.getElementById("refresh-button");
const clockLabelEl = document.getElementById("clock-label");
const backendConfigFormEl = document.getElementById("backend-config-form");
const backendUrlInputEl = document.getElementById("backend-url-input");
const backendResetButtonEl = document.getElementById("backend-reset-button");
const backendCurrentLabelEl = document.getElementById("backend-current-label");

renderNav();
bindGlobalEvents();
syncRouteHeader();
syncBackendUi();
renderCurrentRoute();
connectTelemetrySocket();
refreshData({ force: true });
state.refreshTimer = window.setInterval(() => refreshData({ force: false }), 4000);
window.setInterval(syncClock, 1000);
syncClock();

function getRouteFromLocation() {
  const token = location.hash.replace(/^#/, "") || location.pathname;
  const normalized = token.startsWith("/") ? token : `/${token}`;
  const match = ROUTES.find((route) => normalized === route.path);
  return match ? match.key : "status";
}

function getInitialBackendBase() {
  const params = new URLSearchParams(location.search);
  const queryBackend = normalizeBackendBase(params.get("backend"));
  if (queryBackend) {
    localStorage.setItem("rai_backend_base", queryBackend);
    return queryBackend;
  }

  const stored = normalizeBackendBase(localStorage.getItem("rai_backend_base"));
  return stored || `${location.protocol}//${location.host}`;
}

function getRouteMeta(routeKey = state.route) {
  return ROUTES.find((route) => route.key === routeKey) || ROUTES[0];
}

function renderNav() {
  navEl.innerHTML = ROUTES.map((route) => `
    <a class="nav-link ${route.key === state.route ? "active" : ""}" href="#${route.path}" data-route="${route.key}">
      <strong>${route.title}</strong>
      <span>${route.description}</span>
    </a>
  `).join("");
}

function bindGlobalEvents() {
  navEl.addEventListener("click", (event) => {
    const link = event.target.closest("[data-route]");
    if (!link) {
      return;
    }
    event.preventDefault();
    navigate(link.getAttribute("href"), link.dataset.route);
  });

  refreshButtonEl.addEventListener("click", () => refreshData({ force: true }));

  window.addEventListener("hashchange", () => {
    state.route = getRouteFromLocation();
    renderNav();
    syncRouteHeader();
    renderCurrentRoute();
    refreshData({ force: true });
  });

  backendConfigFormEl.addEventListener("submit", (event) => {
    event.preventDefault();
    const nextBase = normalizeBackendBase(backendUrlInputEl.value);
    if (!nextBase) {
      toast("Backend URL khong hop le", true);
      return;
    }
    setBackendBase(nextBase);
  });

  backendResetButtonEl.addEventListener("click", () => {
    setBackendBase(`${location.protocol}//${location.host}`);
  });

  window.addEventListener("keydown", (event) => {
    if (state.route !== "monitor") {
      return;
    }
    const command = commandFromKeyboard(event.code);
    if (!command) {
      return;
    }
    event.preventDefault();
    if (!state.pressedKeys.has(event.code)) {
      state.pressedKeys.add(event.code);
      sendVelocityCommand(command.vx, command.vy, command.wz);
    }
  });

  window.addEventListener("keyup", (event) => {
    if (!state.pressedKeys.has(event.code)) {
      return;
    }
    state.pressedKeys.delete(event.code);
    stopRobot();
  });
}

function navigate(path, routeKey) {
  if (routeKey === state.route) {
    return;
  }
  cleanupRoute(state.route, routeKey);
  state.route = routeKey;
  location.hash = path;
  renderNav();
  syncRouteHeader();
  renderCurrentRoute();
  refreshData({ force: true });
}

function syncRouteHeader() {
  const route = getRouteMeta();
  pageKickerEl.textContent = route.kicker;
  pageTitleEl.textContent = route.title;
}

function syncClock() {
  clockLabelEl.textContent = new Date().toISOString().slice(11, 19);
}

function syncBackendUi() {
  backendUrlInputEl.value = state.backendBase;
  backendCurrentLabelEl.textContent = `Dang tro toi ${state.backendBase}`;
}

function setBackendBase(nextBase) {
  state.backendBase = nextBase;
  localStorage.setItem("rai_backend_base", nextBase);
  syncBackendUi();
  teardownMonitorVideo();
  closeSocket("telemetry");
  closeSocket("map");
  closeSocket("paths");
  connectTelemetrySocket();
  refreshData({ force: true });
  toast(`Da doi backend sang ${nextBase}`);
}

function setApiStatus(ok, label) {
  state.apiOnline = ok;
  apiStatusDotEl.className = `status-dot ${ok ? "ok" : "error"}`;
  apiStatusLabelEl.textContent = label;
}

function apiUrl(path) {
  return new URL(path, `${state.backendBase}/`).toString();
}

async function requestJson(url, options) {
  const response = await fetch(apiUrl(url), options);
  if (!response.ok) {
    const body = await response.text();
    throw new Error(body || `HTTP ${response.status}`);
  }
  return response.json();
}

async function refreshData({ force }) {
  if (state.refreshInFlight && !force) {
    return;
  }
  state.refreshInFlight = true;

  try {
    const requests = [
      requestJson("/api/system/summary"),
      requestJson("/api/train/status"),
      requestJson("/api/map/list"),
      requestJson("/api/dataset/scenarios"),
      requestJson("/api/dataset/runs"),
      requestJson("/api/system/nodes"),
      requestJson("/api/system/logs?limit=120")
    ];

    const [summary, training, savedMaps, scenarios, datasetRuns, nodesPayload, logsPayload] = await Promise.all(requests);

    state.summary = summary;
    state.training = training;
    state.savedMaps = savedMaps;
    state.scenarios = scenarios;
    state.datasetRuns = datasetRuns;
    state.nodes = nodesPayload.nodes || [];
    state.logs = logsPayload.logs || [];
    state.datasetActive = summary.dataset || null;
    state.telemetry = summary.telemetry || state.telemetry;
    setApiStatus(true, "Online");

    if (state.route === "monitor" || state.route === "slam") {
      ensureMapSocket();
    } else {
      closeSocket("map");
    }

    if (state.route === "monitor") {
      ensurePathsSocket();
      ensureMonitorVideo();
    } else {
      closeSocket("paths");
      teardownMonitorVideo();
    }

    updateCurrentRoute();
  } catch (error) {
    console.error(error);
    setApiStatus(false, "Mat ket noi");
    updateCurrentRoute();
  } finally {
    state.refreshInFlight = false;
  }
}

function connectTelemetrySocket() {
  closeSocket("telemetry");
  const socket = new WebSocket(wsUrl("/api/ws/telemetry"));
  socket.onopen = () => setApiStatus(true, "Streaming");
  socket.onmessage = (event) => {
    const payload = JSON.parse(event.data);
    state.telemetry = payload;
    pushTrailPoint(payload?.odom);
    updateCurrentRoute();
  };
  socket.onerror = () => setApiStatus(false, "WS loi");
  socket.onclose = () => {
    if (state.sockets.telemetry === socket) {
      state.sockets.telemetry = null;
    }
    window.setTimeout(connectTelemetrySocket, 2000);
  };
  state.sockets.telemetry = socket;
}

function ensureMapSocket() {
  if (state.sockets.map) {
    return;
  }
  const socket = new WebSocket(wsUrl("/api/ws/map"));
  socket.onmessage = (event) => {
    state.map = JSON.parse(event.data);
    state.mapRasterCache.key = null;
    updateCurrentRoute();
  };
  socket.onclose = () => {
    if (state.sockets.map === socket) {
      state.sockets.map = null;
      if (state.route === "monitor" || state.route === "slam") {
        window.setTimeout(ensureMapSocket, 1500);
      }
    }
  };
  state.sockets.map = socket;
}

function ensurePathsSocket() {
  if (state.sockets.paths) {
    return;
  }
  const socket = new WebSocket(wsUrl("/api/ws/paths"));
  socket.onmessage = (event) => {
    state.paths = JSON.parse(event.data);
    updateCurrentRoute();
  };
  socket.onclose = () => {
    if (state.sockets.paths === socket) {
      state.sockets.paths = null;
      if (state.route === "monitor") {
        window.setTimeout(ensurePathsSocket, 1500);
      }
    }
  };
  state.sockets.paths = socket;
}

function closeSocket(key) {
  if (state.sockets[key]) {
    state.sockets[key].close();
    state.sockets[key] = null;
  }
}

function cleanupRoute(previousRoute) {
  if (previousRoute === "monitor") {
    teardownMonitorVideo();
    closeSocket("paths");
    state.pressedKeys.clear();
  }
  if (previousRoute === "monitor" || previousRoute === "slam") {
    closeSocket("map");
  }
}

function renderCurrentRoute() {
  switch (state.route) {
    case "status":
      renderStatusPage();
      break;
    case "monitor":
      renderMonitorPage();
      break;
    case "slam":
      renderSlamPage();
      break;
    case "analyst":
      renderAnalystPage();
      break;
    case "dataset":
      renderDatasetPage();
      break;
    case "logs":
      renderLogsPage();
      break;
    default:
      renderStatusPage();
      break;
  }
  updateCurrentRoute();
}

function updateCurrentRoute() {
  switch (state.route) {
    case "status":
      updateStatusPage();
      break;
    case "monitor":
      updateMonitorPage();
      break;
    case "slam":
      updateSlamPage();
      break;
    case "analyst":
      updateAnalystPage();
      break;
    case "dataset":
      updateDatasetPage();
      break;
    case "logs":
      updateLogsPage();
      break;
    default:
      break;
  }
}

function renderStatusPage() {
  appEl.innerHTML = `
    <section class="grid cols-4" id="status-summary-cards"></section>
    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row">
          <span class="pill">He thong runtime</span>
        </div>
        <div class="stat-list" id="runtime-stat-list"></div>
      </article>
      <article class="panel">
        <div class="action-row">
          <span class="pill warning">Node ROS2 dang chay</span>
        </div>
        <div id="status-node-list"></div>
      </article>
    </section>
    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row">
          <span class="pill">Ngu canh an toan</span>
        </div>
        <div id="context-summary"></div>
      </article>
      <article class="panel">
        <div class="action-row">
          <span class="pill">Tien trinh train</span>
        </div>
        <div id="training-summary"></div>
      </article>
    </section>
  `;
}

function updateStatusPage() {
  const summaryCards = document.getElementById("status-summary-cards");
  const runtimeList = document.getElementById("runtime-stat-list");
  const nodeList = document.getElementById("status-node-list");
  const contextSummary = document.getElementById("context-summary");
  const trainingSummary = document.getElementById("training-summary");
  if (!summaryCards || !runtimeList || !nodeList || !contextSummary || !trainingSummary) {
    return;
  }

  const telemetry = state.telemetry || state.summary?.telemetry || {};
  const runtime = state.summary?.runtime || {};
  const battery = telemetry.battery || {};
  const odom = telemetry.odom || {};
  const context = telemetry.context || {};

  summaryCards.innerHTML = [
    metricCard("Battery", `${formatNumber(battery.percentage, 1)}%`, `Voltage ${formatNumber(battery.voltage, 2)}V`, battery.percentage > 30 ? "On dinh" : "Can sac"),
    metricCard("Pose", `${formatNumber(odom.x, 2)}, ${formatNumber(odom.y, 2)}`, `Yaw ${formatNumber(radiansToDegrees(odom.theta), 1)} deg`, "Ban do map"),
    metricCard("Dataset", state.datasetActive?.active ? "Dang ghi" : "Ranh", state.datasetActive?.run?.run_name || "Khong co run", state.summary?.counts?.dataset_runs + " run"),
    metricCard("SLAM / Nav2", state.summary?.slam_active ? "SLAM on" : "SLAM off", runtime.nav2_available ? "Nav2 available" : "Nav2 unavailable", runtime.latest_map_available ? "Co map live" : "Chua co map")
  ].join("");

  runtimeList.innerHTML = `
    ${statRow("Telemetry clients", runtime.telemetry_clients)}
    ${statRow("Camera clients", runtime.camera_clients)}
    ${statRow("Map clients", runtime.map_clients)}
    ${statRow("Paths clients", runtime.paths_clients)}
    ${statRow("Saved maps", state.summary?.counts?.saved_maps || 0)}
    ${statRow("Humans tracked", Array.isArray(telemetry.humans) ? telemetry.humans.length : 0)}
  `;

  nodeList.innerHTML = state.nodes.length
    ? state.nodes.slice(0, 10).map((node) => `
        <div class="stat-row">
          <div>
            <strong>${escapeHtml(node.node_name)}</strong>
            <div class="muted">${escapeHtml(node.package_hint || "unknown package")}</div>
          </div>
          <span class="pill">active</span>
        </div>
      `).join("")
    : emptyState("Chua doc duoc danh sach node ROS2.");

  contextSummary.innerHTML = `
    <div class="telemetry-strip">
      <div class="mini-card"><span class="muted">Legacy context</span><strong>${escapeHtml(context.legacy_context || "N/A")}</strong></div>
      <div class="mini-card"><span class="muted">phi_h</span><strong>${formatNumber(context.phi_h, 3)}</strong></div>
      <div class="mini-card"><span class="muted">d_safe</span><strong>${formatNumber(context.d_safe, 2)} m</strong></div>
    </div>
    <div class="stat-list">
      ${statRow("vx_max", `${formatNumber(context.vx_max, 2)} m/s`)}
      ${statRow("vy_max", `${formatNumber(context.vy_max, 2)} m/s`)}
      ${statRow("omega_max", `${formatNumber(context.omega_max, 2)} rad/s`)}
      ${statRow("Occlusion flag", context.occlusion_flag ? "True" : "False")}
      ${statRow("Lidar left clearance", `${formatNumber(telemetry.lidar_clearance?.left, 2)} m`)}
      ${statRow("Lidar right clearance", `${formatNumber(telemetry.lidar_clearance?.right, 2)} m`)}
    </div>
  `;

  trainingSummary.innerHTML = `
    <div class="stat-list">
      ${statRow("Running", state.training?.running ? "True" : "False")}
      ${statRow("Epoch", state.training?.epoch ?? 0)}
      ${statRow("Loss", formatNumber(state.training?.loss, 4))}
      ${statRow("Accuracy", `${formatNumber((state.training?.accuracy || 0) * 100, 1)}%`)}
    </div>
    <div class="progress"><span style="width:${state.training?.progress || 0}%"></span></div>
  `;
}

function renderMonitorPage() {
  appEl.innerHTML = `
    <section class="split">
      <article class="video-card">
        <div class="action-row">
          <span class="pill">WebRTC camera</span>
          <span class="pill warning" id="monitor-video-status">Dang ket noi</span>
        </div>
        <div class="video-frame">
          <video id="monitor-video" autoplay playsinline muted></video>
          <div class="video-overlay" id="monitor-overlay"></div>
        </div>
        <p class="muted" id="monitor-detect-note">Overlay bbox se hien thi neu feed detection cung cap toa do bbox.</p>
      </article>
      <div class="stack">
        <article class="panel">
          <div class="action-row"><span class="pill">Detect / safety</span></div>
          <div id="monitor-human-list"></div>
        </article>
        <article class="panel">
          <div class="action-row"><span class="pill">Robot telemetry</span></div>
          <div id="monitor-telemetry"></div>
        </article>
      </div>
    </section>

    <section class="canvas-card">
      <div class="action-row">
        <span class="pill">Map realtime Nav2</span>
        <div class="legend">
          <div class="legend-item"><span class="legend-swatch" style="background:#0d7f66;"></span>Global plan</div>
          <div class="legend-item"><span class="legend-swatch" style="background:#ff8a3d;"></span>Local plan</div>
          <div class="legend-item"><span class="legend-swatch" style="background:#154a8d;"></span>Duong da di</div>
        </div>
      </div>
      <canvas id="monitor-map" width="1280" height="720"></canvas>
      <div class="muted" id="monitor-map-meta"></div>
    </section>

    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row"><span class="pill">Manual control</span></div>
        <div class="control-grid" id="control-grid">
          <button type="button" data-cmd="forward">▲</button>
          <button type="button" data-cmd="stop">■</button>
          <button type="button" data-cmd="rotate-left">⟲</button>
          <button type="button" data-cmd="left">◀</button>
          <button type="button" data-cmd="backward">▼</button>
          <button type="button" data-cmd="right">▶</button>
        </div>
        <p class="muted">Ho tro phim W A S D va Q/E de quay. Nha phim se gui lenh dung.</p>
      </article>
      <article class="panel">
        <div class="action-row"><span class="pill">Nav2 goal</span></div>
        <form class="form-grid" id="nav-goal-form">
          <div class="field"><label>X (m)</label><input name="x" type="number" step="0.1" value="1.0"></div>
          <div class="field"><label>Y (m)</label><input name="y" type="number" step="0.1" value="1.0"></div>
          <div class="field full"><label>Yaw (rad)</label><input name="yaw" type="number" step="0.1" value="0"></div>
          <div class="field full action-row">
            <button class="primary-button" type="submit">Gui muc tieu</button>
            <button class="danger-button" id="cancel-nav-button" type="button">Huy Nav2</button>
          </div>
        </form>
      </article>
    </section>
  `;

  const controlGrid = document.getElementById("control-grid");
  controlGrid.addEventListener("pointerdown", (event) => {
    const button = event.target.closest("button[data-cmd]");
    if (!button) {
      return;
    }
    const cmd = controlPreset(button.dataset.cmd);
    sendVelocityCommand(cmd.vx, cmd.vy, cmd.wz);
  });
  controlGrid.addEventListener("pointerup", stopRobot);
  controlGrid.addEventListener("pointerleave", stopRobot);

  document.getElementById("nav-goal-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    try {
      await requestJson("/api/robot/nav/goal", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          x: Number(form.get("x")),
          y: Number(form.get("y")),
          yaw: Number(form.get("yaw"))
        })
      });
      toast("Da gui Nav2 goal");
    } catch (error) {
      toast("Gui goal that bai", true);
    }
  });

  document.getElementById("cancel-nav-button").addEventListener("click", async () => {
    try {
      await requestJson("/api/robot/nav/cancel", { method: "POST" });
      toast("Da huy goal Nav2");
    } catch (error) {
      toast("Huy goal that bai", true);
    }
  });
}

function updateMonitorPage() {
  const humanList = document.getElementById("monitor-human-list");
  const telemetryBox = document.getElementById("monitor-telemetry");
  const overlay = document.getElementById("monitor-overlay");
  const note = document.getElementById("monitor-detect-note");
  const meta = document.getElementById("monitor-map-meta");
  const videoStatus = document.getElementById("monitor-video-status");
  if (!humanList || !telemetryBox || !overlay || !note || !meta || !videoStatus) {
    return;
  }

  const telemetry = state.telemetry || {};
  const humans = Array.isArray(telemetry.humans) ? telemetry.humans : [];
  const context = telemetry.context || {};
  const odom = telemetry.odom || {};

  humanList.innerHTML = humans.length
    ? humans.map((human, index) => `
        <div class="stat-row">
          <div>
            <strong>Nguoi ${index + 1}</strong>
            <div class="muted">${escapeHtml(JSON.stringify(human))}</div>
          </div>
          <span class="pill ${context.occlusion_flag ? "danger" : ""}">${escapeHtml(context.legacy_context || "context")}</span>
        </div>
      `).join("")
    : emptyState("Chua co doi tuong detect tu topic /canmpc/humans.");

  telemetryBox.innerHTML = `
    <div class="telemetry-strip">
      <div class="mini-card"><span class="muted">X / Y</span><strong>${formatNumber(odom.x, 2)} / ${formatNumber(odom.y, 2)}</strong></div>
      <div class="mini-card"><span class="muted">Theta</span><strong>${formatNumber(radiansToDegrees(odom.theta), 1)} deg</strong></div>
      <div class="mini-card"><span class="muted">Battery</span><strong>${formatNumber(telemetry.battery?.percentage, 1)}%</strong></div>
    </div>
    <div class="stat-list">
      ${statRow("v_x", `${formatNumber(odom.linear_x, 2)} m/s`)}
      ${statRow("v_y", `${formatNumber(odom.linear_y, 2)} m/s`)}
      ${statRow("w_z", `${formatNumber(odom.angular_z, 2)} rad/s`)}
      ${statRow("phi_h", formatNumber(context.phi_h, 3))}
      ${statRow("d_safe", `${formatNumber(context.d_safe, 2)} m`)}
      ${statRow("Occlusion", context.occlusion_flag ? "True" : "False")}
    </div>
  `;

  overlay.innerHTML = "";
  let overlayCount = 0;
  humans.forEach((human, index) => {
    const bbox = human.bbox || human.bbox_norm || human.bounding_box;
    if (!bbox) {
      return;
    }
    const normalized = normalizeBbox(bbox);
    if (!normalized) {
      return;
    }
    overlayCount += 1;
    const box = document.createElement("div");
    box.className = "detect-box";
    box.style.left = `${normalized.x * 100}%`;
    box.style.top = `${normalized.y * 100}%`;
    box.style.width = `${normalized.w * 100}%`;
    box.style.height = `${normalized.h * 100}%`;
    box.innerHTML = `<label>${escapeHtml(human.label || human.id || `human-${index + 1}`)}</label>`;
    overlay.appendChild(box);
  });
  note.textContent = overlayCount
    ? `${overlayCount} bbox dang hien thi tren stream.`
    : "Feed hien tai chua cung cap bbox 2D. Trang van san sang overlay ngay khi co truong bbox.";

  videoStatus.textContent = state.monitorStream ? "Video live" : "Dang cho stream";
  meta.textContent = state.map
    ? `Map ${state.map.width}x${state.map.height} | res ${formatNumber(state.map.resolution, 3)} m/pixel | global ${state.paths.global_plan.length} pts | local ${state.paths.local_plan.length} pts | trail ${state.monitorTrail.length} pts`
    : "Dang cho /map tu SLAM hoac map_server.";

  drawMap("monitor-map", state.map, state.paths, state.monitorTrail);
}

function renderSlamPage() {
  appEl.innerHTML = `
    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row">
          <span class="pill">SLAM control</span>
        </div>
        <div class="action-row">
          <button class="primary-button" id="slam-start-button" type="button">Start SLAM</button>
          <button class="danger-button" id="slam-stop-button" type="button">Stop SLAM</button>
        </div>
        <form class="form-grid" id="save-map-form">
          <div class="field full">
            <label>Ten map</label>
            <input name="name" type="text" placeholder="warehouse_2026_06_15">
          </div>
          <div class="field full action-row">
            <button class="primary-button" type="submit">Luu map hien tai</button>
          </div>
        </form>
      </article>
      <article class="panel">
        <div class="action-row"><span class="pill warning">Saved maps</span></div>
        <div id="saved-map-list"></div>
      </article>
    </section>
    <section class="canvas-card">
      <div class="action-row"><span class="pill">Live occupancy grid</span></div>
      <canvas id="slam-map" width="1280" height="720"></canvas>
      <div class="muted" id="slam-map-meta"></div>
    </section>
  `;

  document.getElementById("slam-start-button").addEventListener("click", async () => {
    try {
      await requestJson("/api/robot/slam/start", { method: "POST" });
      toast("Da danh dau SLAM = active");
      refreshData({ force: true });
    } catch (error) {
      toast("Khong the start SLAM", true);
    }
  });

  document.getElementById("slam-stop-button").addEventListener("click", async () => {
    try {
      await requestJson("/api/robot/slam/stop", { method: "POST" });
      toast("Da danh dau SLAM = inactive");
      refreshData({ force: true });
    } catch (error) {
      toast("Khong the stop SLAM", true);
    }
  });

  document.getElementById("save-map-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    try {
      await requestJson("/api/map/save", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ name: form.get("name") })
      });
      toast("Da luu map");
      event.currentTarget.reset();
      refreshData({ force: true });
    } catch (error) {
      toast("Luu map that bai", true);
    }
  });
}

function updateSlamPage() {
  const list = document.getElementById("saved-map-list");
  const meta = document.getElementById("slam-map-meta");
  if (!list || !meta) {
    return;
  }

  list.innerHTML = state.savedMaps.length
    ? state.savedMaps.slice(0, 8).map((item) => `
        <div class="stat-row">
          <div>
            <strong>${escapeHtml(item.name)}</strong>
            <div class="muted">${escapeHtml(item.created_at || "")}</div>
          </div>
          <div class="muted">${item.width}x${item.height}</div>
        </div>
      `).join("")
    : emptyState("Chua co map da luu trong co so du lieu.");

  meta.textContent = state.map
    ? `Map live ${state.map.width}x${state.map.height} | origin (${formatNumber(state.map.origin_x, 2)}, ${formatNumber(state.map.origin_y, 2)})`
    : "Dang cho du lieu /map.";
  drawMap("slam-map", state.map, { global_plan: [], local_plan: [] }, []);
}

function renderAnalystPage() {
  appEl.innerHTML = `
    <section class="grid cols-4" id="analyst-summary-cards"></section>
    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row"><span class="pill">Training history</span></div>
        <div id="training-chart-wrap"></div>
      </article>
      <article class="panel">
        <div class="action-row"><span class="pill warning">Dataset metric aggregate</span></div>
        <div id="analyst-metric-list"></div>
      </article>
    </section>
    <section class="table-wrap">
      <div class="action-row"><span class="pill">Recent runs</span></div>
      <div id="analyst-runs-table"></div>
    </section>
  `;
}

function updateAnalystPage() {
  const cards = document.getElementById("analyst-summary-cards");
  const chartWrap = document.getElementById("training-chart-wrap");
  const metricList = document.getElementById("analyst-metric-list");
  const runsTable = document.getElementById("analyst-runs-table");
  if (!cards || !chartWrap || !metricList || !runsTable) {
    return;
  }

  const completedRuns = state.datasetRuns.filter((run) => run.status === "COMPLETED" || run.status === "COMPRESSED");
  const avgSamples = average(completedRuns.map((run) => run.samples_count || 0));
  const avgTimeout = average(completedRuns.map((run) => run.timeout_rate || 0));
  const minClearance = minimum(completedRuns.map((run) => run.min_human_clearance));
  const avgRmse = average(completedRuns.map((run) => run.metrics?.tracking?.rmse_xy || 0));

  cards.innerHTML = [
    metricCard("Completed runs", completedRuns.length, "Tat ca dataset run hoan tat", `${state.datasetRuns.length} tong run`),
    metricCard("Avg samples", Math.round(avgSamples || 0), "Trung binh moi run", "samples"),
    metricCard("Min clearance", minClearance != null ? `${formatNumber(minClearance, 2)} m` : "N/A", "Thap nhat trong run", "safety"),
    metricCard("Avg RMSE XY", avgRmse ? `${formatNumber(avgRmse, 3)} m` : "N/A", "Tracking metric", `timeout ${formatNumber(avgTimeout * 100, 1)}%`)
  ].join("");

  chartWrap.innerHTML = state.training?.history?.length
    ? renderTrainingChart(state.training.history)
    : emptyState("Chua co lich su huan luyen.");

  metricList.innerHTML = `
    ${statRow("Avg timeout rate", `${formatNumber(avgTimeout * 100, 1)}%`)}
    ${statRow("Best battery end", `${formatNumber(maximum(completedRuns.map((run) => run.end_voltage)), 2)} V`)}
    ${statRow("Mean battery percent", `${formatNumber(average(completedRuns.map((run) => run.avg_percentage || 0)), 1)}%`)}
    ${statRow("Worst phi_h max", formatNumber(maximum(completedRuns.map((run) => run.phi_h_max)), 3))}
    ${statRow("Avg run duration", `${formatNumber(average(completedRuns.map((run) => run.duration || 0)), 1)} s`)}
  `;

  runsTable.innerHTML = state.datasetRuns.length
    ? `
        <table>
          <thead>
            <tr><th>Run</th><th>Scenario</th><th>Status</th><th>Samples</th><th>RMSE XY</th><th>d_min</th><th>Timeout</th></tr>
          </thead>
          <tbody>
            ${state.datasetRuns.slice(0, 12).map((run) => `
              <tr>
                <td>${escapeHtml(run.run_name)}</td>
                <td>${escapeHtml(run.controller_id)}<div class="muted">${escapeHtml(run.environment)}</div></td>
                <td>${escapeHtml(run.status)}</td>
                <td>${run.samples_count || 0}</td>
                <td>${formatNumber(run.metrics?.tracking?.rmse_xy, 3)}</td>
                <td>${formatNumber(run.metrics?.safety?.d_min ?? run.min_human_clearance, 3)}</td>
                <td>${formatNumber((run.timeout_rate || 0) * 100, 1)}%</td>
              </tr>
            `).join("")}
          </tbody>
        </table>
      `
    : emptyState("Chua co dataset run de phan tich.");
}

function renderDatasetPage() {
  appEl.innerHTML = `
    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row"><span class="pill">Start dataset run</span></div>
        <form class="form-grid" id="dataset-start-form">
          <div class="field">
            <label>Scenario</label>
            <select name="scenario_name" id="scenario-select"></select>
          </div>
          <div class="field">
            <label>Controller</label>
            <input name="controller_id" type="text" value="CCA_NMPC">
          </div>
          <div class="field">
            <label>Environment</label>
            <select name="environment">
              <option value="real">real</option>
              <option value="sim">sim</option>
            </select>
          </div>
          <div class="field">
            <label>Split</label>
            <select name="split">
              <option value="unsplit">unsplit</option>
              <option value="train">train</option>
              <option value="val">val</option>
              <option value="test">test</option>
            </select>
          </div>
          <div class="field full">
            <label>Notes</label>
            <textarea name="notes" placeholder="Mo ta kich ban, actor, dieu kien thu thap"></textarea>
          </div>
          <div class="field full action-row">
            <button class="primary-button" type="submit">Bat dau run</button>
            <button class="danger-button" id="dataset-stop-button" type="button">Dung run</button>
          </div>
        </form>
      </article>
      <article class="panel">
        <div class="action-row"><span class="pill warning">Capture metadata</span></div>
        <form class="form-grid" id="dataset-capture-form">
          <div class="field"><label>Class</label><input name="class_name" type="text" value="human"></div>
          <div class="field"><label>Label</label><input name="label" type="text" value="normal"></div>
          <div class="field full"><label>Tag</label><input name="tag" type="text" value="corridor"></div>
          <div class="field full action-row">
            <button class="primary-button" type="submit">Capture metadata</button>
          </div>
        </form>
      </article>
    </section>

    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row"><span class="pill">Run active</span></div>
        <div id="dataset-active-panel"></div>
      </article>
      <article class="panel">
        <div class="action-row"><span class="pill">Scenario mo ta</span></div>
        <div id="dataset-scenario-panel"></div>
      </article>
    </section>

    <section class="table-wrap">
      <div class="action-row"><span class="pill">Danh sach run</span></div>
      <div id="dataset-runs-table"></div>
    </section>
  `;

  document.getElementById("dataset-start-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    const payload = Object.fromEntries(form.entries());
    try {
      await requestJson("/api/dataset/start", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      toast("Da bat dau dataset run");
      refreshData({ force: true });
    } catch (error) {
      toast("Bat dau run that bai", true);
    }
  });

  document.getElementById("dataset-stop-button").addEventListener("click", async () => {
    try {
      await requestJson("/api/dataset/stop", { method: "POST" });
      toast("Da dung run dataset");
      refreshData({ force: true });
    } catch (error) {
      toast("Dung run that bai", true);
    }
  });

  document.getElementById("dataset-capture-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    try {
      const payload = Object.fromEntries(form.entries());
      const result = await requestJson("/api/dataset/capture", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      toast(`Capture: ${result.tag}`);
    } catch (error) {
      toast("Capture that bai", true);
    }
  });
}

function updateDatasetPage() {
  const scenarioSelect = document.getElementById("scenario-select");
  const activePanel = document.getElementById("dataset-active-panel");
  const scenarioPanel = document.getElementById("dataset-scenario-panel");
  const runsTable = document.getElementById("dataset-runs-table");
  if (!scenarioSelect || !activePanel || !scenarioPanel || !runsTable) {
    return;
  }

  if (!scenarioSelect.dataset.initialized) {
    scenarioSelect.innerHTML = state.scenarios.map((scenario) => `
      <option value="${escapeHtml(scenario.name)}">${escapeHtml(scenario.name)}</option>
    `).join("");
    scenarioSelect.dataset.initialized = "true";
  }

  activePanel.innerHTML = state.datasetActive?.active
    ? `
        <div class="stat-list">
          ${statRow("Run", state.datasetActive.run?.run_name || "N/A")}
          ${statRow("Scenario", state.datasetActive.run?.scenario_name || "N/A")}
          ${statRow("Controller", state.datasetActive.run?.controller_id || "N/A")}
          ${statRow("Started", state.datasetActive.run?.start_time || "N/A")}
          ${statRow("Path", state.datasetActive.run?.data_path || "N/A")}
        </div>
      `
    : emptyState("Khong co dataset run dang ghi.");

  scenarioPanel.innerHTML = state.scenarios.length
    ? state.scenarios.map((scenario) => `
        <div class="stat-row">
          <div>
            <strong>${escapeHtml(scenario.name)}</strong>
            <div class="muted">${escapeHtml(scenario.description || "")}</div>
          </div>
          <div class="muted">${escapeHtml(scenario.difficulty || "")}</div>
        </div>
      `).join("")
    : emptyState("Chua co scenario metadata.");

  runsTable.innerHTML = state.datasetRuns.length
    ? `
        <table>
          <thead>
            <tr><th>Run</th><th>Status</th><th>Scenario</th><th>Samples</th><th>Duration</th><th>Archive</th></tr>
          </thead>
          <tbody>
            ${state.datasetRuns.slice(0, 15).map((run) => `
              <tr>
                <td>${escapeHtml(run.run_name)}</td>
                <td>${escapeHtml(run.status)}</td>
                <td>${escapeHtml(run.controller_id)}<div class="muted">${escapeHtml(run.environment)} / ${escapeHtml(run.split)}</div></td>
                <td>${run.samples_count || 0}</td>
                <td>${formatNumber(run.duration, 1)} s</td>
                <td>${run.zip_path ? `<a class="pill" href="${apiUrl(`/api/dataset/download/${run.id}`)}" target="_blank" rel="noreferrer">Tai ZIP</a>` : "<span class='muted'>Dang tao</span>"}</td>
              </tr>
            `).join("")}
          </tbody>
        </table>
      `
    : emptyState("Chua co run nao.");
}

function renderLogsPage() {
  appEl.innerHTML = `
    <section class="grid cols-2">
      <article class="panel">
        <div class="action-row"><span class="pill">ROS2 nodes</span></div>
        <div id="logs-node-list"></div>
      </article>
      <article class="panel">
        <div class="action-row"><span class="pill warning">Runtime status</span></div>
        <div id="logs-runtime-summary"></div>
      </article>
    </section>
    <section class="table-wrap">
      <div class="action-row"><span class="pill">Logs</span></div>
      <div id="logs-list"></div>
    </section>
  `;
}

function updateLogsPage() {
  const nodeList = document.getElementById("logs-node-list");
  const runtimeSummary = document.getElementById("logs-runtime-summary");
  const logsList = document.getElementById("logs-list");
  if (!nodeList || !runtimeSummary || !logsList) {
    return;
  }

  nodeList.innerHTML = state.nodes.length
    ? state.nodes.map((node) => `
        <div class="stat-row">
          <div>
            <strong>${escapeHtml(node.node_name)}</strong>
            <div class="muted">${escapeHtml(node.package_hint || "unknown")}</div>
          </div>
          <span class="pill">active</span>
        </div>
      `).join("")
    : emptyState("Khong co node nao duoc tra ve.");

  const runtime = state.summary?.runtime || {};
  runtimeSummary.innerHTML = `
    ${statRow("API status", state.apiOnline ? "Online" : "Offline")}
    ${statRow("SLAM active", state.summary?.slam_active ? "True" : "False")}
    ${statRow("Map live", runtime.latest_map_available ? "True" : "False")}
    ${statRow("Nav2 ready", runtime.nav2_available ? "True" : "False")}
    ${statRow("Last telemetry", state.telemetry?.last_update ? new Date(state.telemetry.last_update * 1000).toISOString() : "N/A")}
  `;

  logsList.innerHTML = state.logs.length
    ? state.logs.map((log) => `
        <div class="log-entry">
          <div class="log-meta">
            <span class="severity ${escapeHtml(log.severity)}">${escapeHtml(log.severity)}</span>
            <span>${escapeHtml(log.timestamp || "")}</span>
            <span>${escapeHtml(log.source || "")}</span>
          </div>
          <div>${escapeHtml(log.message || "")}</div>
          <div class="muted">${escapeHtml(log.path || "")}</div>
        </div>
      `).join("")
    : emptyState("Chua doc duoc log runtime.");
}

async function ensureMonitorVideo() {
  const videoEl = document.getElementById("monitor-video");
  if (!videoEl || state.monitorStream || state.peerConnection) {
    return;
  }

  try {
    const pc = new RTCPeerConnection();
    pc.addTransceiver("video", { direction: "recvonly" });
    pc.ontrack = (event) => {
      state.monitorStream = event.streams[0];
      videoEl.srcObject = state.monitorStream;
      updateCurrentRoute();
    };

    const offer = await pc.createOffer();
    await pc.setLocalDescription(offer);
    const answer = await requestJson("/api/webrtc/offer", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ sdp: offer.sdp, type: offer.type })
    });
    await pc.setRemoteDescription(answer);
    state.peerConnection = pc;
  } catch (error) {
    console.error(error);
  }
}

function teardownMonitorVideo() {
  if (state.peerConnection) {
    state.peerConnection.close();
    state.peerConnection = null;
  }
  state.monitorStream = null;
  const videoEl = document.getElementById("monitor-video");
  if (videoEl) {
    videoEl.srcObject = null;
  }
}

async function sendVelocityCommand(vx, vy, wz) {
  state.activeControl = vx === 0 && vy === 0 && wz === 0 ? null : { vx, vy, wz };
  try {
    await requestJson("/api/robot/cmd_vel", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ linear_x: vx, linear_y: vy, angular_z: wz })
    });
  } catch (error) {
    toast("Gui cmd_vel that bai", true);
  }
}

function stopRobot() {
  if (!state.activeControl) {
    return;
  }
  state.activeControl = null;
  sendVelocityCommand(0, 0, 0);
}

function controlPreset(name) {
  const presets = {
    forward: { vx: 0.35, vy: 0, wz: 0 },
    backward: { vx: -0.3, vy: 0, wz: 0 },
    left: { vx: 0, vy: 0.3, wz: 0 },
    right: { vx: 0, vy: -0.3, wz: 0 },
    "rotate-left": { vx: 0, vy: 0, wz: 0.7 },
    stop: { vx: 0, vy: 0, wz: 0 }
  };
  return presets[name] || presets.stop;
}

function commandFromKeyboard(code) {
  const keyboard = {
    KeyW: { vx: 0.35, vy: 0, wz: 0 },
    KeyS: { vx: -0.3, vy: 0, wz: 0 },
    KeyA: { vx: 0, vy: 0.3, wz: 0 },
    KeyD: { vx: 0, vy: -0.3, wz: 0 },
    KeyQ: { vx: 0, vy: 0, wz: 0.7 },
    KeyE: { vx: 0, vy: 0, wz: -0.7 }
  };
  return keyboard[code] || null;
}

function pushTrailPoint(odom) {
  if (!odom || typeof odom.x !== "number" || typeof odom.y !== "number") {
    return;
  }
  const last = state.monitorTrail[state.monitorTrail.length - 1];
  const point = { x: odom.x, y: odom.y };
  if (!last || distance(last, point) > 0.03) {
    state.monitorTrail.push(point);
  }
  if (state.monitorTrail.length > 1200) {
    state.monitorTrail.splice(0, state.monitorTrail.length - 1200);
  }
}

function drawMap(canvasId, map, paths, trail) {
  const canvas = document.getElementById(canvasId);
  if (!canvas) {
    return;
  }
  const ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = "#fbfcfb";
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  if (!map) {
    ctx.fillStyle = "#60726c";
    ctx.font = "20px IBM Plex Sans, Segoe UI, sans-serif";
    ctx.fillText("Dang cho occupancy grid /map ...", 36, 48);
    return;
  }

  const raster = getMapRaster(map);
  const scale = Math.min(canvas.width / map.width, canvas.height / map.height);
  const drawWidth = map.width * scale;
  const drawHeight = map.height * scale;
  const offsetX = (canvas.width - drawWidth) / 2;
  const offsetY = (canvas.height - drawHeight) / 2;

  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(raster, offsetX, offsetY, drawWidth, drawHeight);
  ctx.strokeStyle = "rgba(20, 32, 28, 0.1)";
  ctx.strokeRect(offsetX, offsetY, drawWidth, drawHeight);

  drawPath(ctx, map, paths?.global_plan || [], offsetX, offsetY, scale, "#0d7f66", 3);
  drawPath(ctx, map, paths?.local_plan || [], offsetX, offsetY, scale, "#ff8a3d", 3);
  drawPath(ctx, map, trail || [], offsetX, offsetY, scale, "#154a8d", 2.5);
}

function getMapRaster(map) {
  const cacheKey = `${map.timestamp}-${map.width}-${map.height}`;
  if (state.mapRasterCache.key === cacheKey && state.mapRasterCache.canvas) {
    return state.mapRasterCache.canvas;
  }

  const raw = atob(map.grid_data);
  const bytes = new Uint8Array(raw.length);
  for (let index = 0; index < raw.length; index += 1) {
    bytes[index] = raw.charCodeAt(index);
  }

  const offscreen = document.createElement("canvas");
  offscreen.width = map.width;
  offscreen.height = map.height;
  const imageData = offscreen.getContext("2d").createImageData(map.width, map.height);

  for (let y = 0; y < map.height; y += 1) {
    for (let x = 0; x < map.width; x += 1) {
      const sourceIndex = x + y * map.width;
      const flippedY = map.height - 1 - y;
      const targetIndex = (x + flippedY * map.width) * 4;
      const value = bytes[sourceIndex];
      let shade = 236;

      if (value === 255) {
        shade = 208;
      } else {
        const occupancy = Math.min(100, value);
        shade = 255 - Math.round((occupancy / 100) * 255);
      }

      imageData.data[targetIndex] = shade;
      imageData.data[targetIndex + 1] = shade;
      imageData.data[targetIndex + 2] = shade;
      imageData.data[targetIndex + 3] = 255;
    }
  }

  offscreen.getContext("2d").putImageData(imageData, 0, 0);
  state.mapRasterCache = { key: cacheKey, canvas: offscreen };
  return offscreen;
}

function drawPath(ctx, map, points, offsetX, offsetY, scale, color, width) {
  if (!Array.isArray(points) || points.length < 2) {
    return;
  }
  ctx.save();
  ctx.beginPath();
  points.forEach((point, index) => {
    const px = offsetX + ((point.x - map.origin_x) / map.resolution) * scale;
    const py = offsetY + (map.height - ((point.y - map.origin_y) / map.resolution)) * scale;
    if (index === 0) {
      ctx.moveTo(px, py);
    } else {
      ctx.lineTo(px, py);
    }
  });
  ctx.strokeStyle = color;
  ctx.lineWidth = width;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";
  ctx.stroke();
  ctx.restore();
}

function renderTrainingChart(history) {
  const lossPoints = history.map((item) => item.loss);
  const accPoints = history.map((item) => item.accuracy);
  return `
    <div class="stack">
      <div class="legend">
        <div class="legend-item"><span class="legend-swatch" style="background:#0d7f66;"></span>Loss</div>
        <div class="legend-item"><span class="legend-swatch" style="background:#ff8a3d;"></span>Accuracy</div>
      </div>
      <div class="chart">${lineChartSvg(lossPoints, accPoints)}</div>
    </div>
  `;
}

function lineChartSvg(lossPoints, accPoints) {
  const width = 760;
  const height = 220;
  const padding = 20;
  const allValues = [...lossPoints, ...accPoints];
  const min = Math.min(...allValues);
  const max = Math.max(...allValues);
  const project = (value, index, total) => {
    const x = padding + (index / Math.max(1, total - 1)) * (width - padding * 2);
    const range = max - min || 1;
    const y = height - padding - ((value - min) / range) * (height - padding * 2);
    return `${x},${y}`;
  };
  const lossPolyline = lossPoints.map((value, index) => project(value, index, lossPoints.length)).join(" ");
  const accPolyline = accPoints.map((value, index) => project(value, index, accPoints.length)).join(" ");
  return `
    <svg viewBox="0 0 ${width} ${height}" width="100%" height="100%" preserveAspectRatio="none">
      <polyline fill="none" stroke="#0d7f66" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" points="${lossPolyline}"></polyline>
      <polyline fill="none" stroke="#ff8a3d" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" points="${accPolyline}"></polyline>
    </svg>
  `;
}

function metricCard(title, value, metaLeft, metaRight) {
  return `
    <article class="card metric-card">
      <span class="eyebrow">${escapeHtml(title)}</span>
      <div class="value">${escapeHtml(String(value))}</div>
      <div class="meta">
        <span>${escapeHtml(String(metaLeft))}</span>
        <span>${escapeHtml(String(metaRight))}</span>
      </div>
    </article>
  `;
}

function statRow(label, value) {
  return `
    <div class="stat-row">
      <span class="muted">${escapeHtml(String(label))}</span>
      <strong>${escapeHtml(String(value ?? "N/A"))}</strong>
    </div>
  `;
}

function emptyState(message) {
  return `<div class="empty-state">${escapeHtml(message)}</div>`;
}

function normalizeBbox(bbox) {
  if (Array.isArray(bbox) && bbox.length >= 4) {
    const [x, y, w, h] = bbox;
    return scaleBox(x, y, w, h);
  }
  if (typeof bbox === "object" && bbox !== null) {
    const x = bbox.x ?? bbox.left ?? 0;
    const y = bbox.y ?? bbox.top ?? 0;
    const w = bbox.w ?? bbox.width;
    const h = bbox.h ?? bbox.height;
    return scaleBox(x, y, w, h);
  }
  return null;
}

function scaleBox(x, y, w, h) {
  if ([x, y, w, h].some((value) => typeof value !== "number")) {
    return null;
  }
  if (x > 1 || y > 1 || w > 1 || h > 1) {
    return null;
  }
  return { x, y, w, h };
}

function wsUrl(path) {
  const backend = new URL(`${state.backendBase}/`);
  const protocol = backend.protocol === "https:" ? "wss:" : "ws:";
  return `${protocol}//${backend.host}${path}`;
}

function normalizeBackendBase(value) {
  if (!value) {
    return null;
  }

  const trimmed = String(value).trim();
  if (!trimmed) {
    return null;
  }

  const withProtocol = /^https?:\/\//i.test(trimmed) ? trimmed : `http://${trimmed}`;
  try {
    const url = new URL(withProtocol);
    return `${url.protocol}//${url.host}`;
  } catch (error) {
    return null;
  }
}

function radiansToDegrees(value) {
  return (value || 0) * 180 / Math.PI;
}

function formatNumber(value, digits = 2) {
  if (value == null || Number.isNaN(Number(value))) {
    return "N/A";
  }
  return Number(value).toFixed(digits);
}

function average(values) {
  const filtered = values.filter((value) => typeof value === "number" && !Number.isNaN(value));
  if (!filtered.length) {
    return 0;
  }
  return filtered.reduce((sum, value) => sum + value, 0) / filtered.length;
}

function maximum(values) {
  const filtered = values.filter((value) => typeof value === "number" && !Number.isNaN(value));
  return filtered.length ? Math.max(...filtered) : null;
}

function minimum(values) {
  const filtered = values.filter((value) => typeof value === "number" && !Number.isNaN(value));
  return filtered.length ? Math.min(...filtered) : null;
}

function distance(a, b) {
  return Math.hypot((a.x || 0) - (b.x || 0), (a.y || 0) - (b.y || 0));
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function toast(message, isError = false) {
  const existing = document.getElementById("app-toast");
  if (existing) {
    existing.remove();
  }
  const toastEl = document.createElement("div");
  toastEl.id = "app-toast";
  toastEl.className = "sidebar-footer";
  toastEl.style.position = "fixed";
  toastEl.style.left = "auto";
  toastEl.style.right = "24px";
  toastEl.style.bottom = "24px";
  toastEl.style.zIndex = "9999";
  toastEl.innerHTML = `
    <div class="status-dot ${isError ? "error" : "ok"}"></div>
    <div><strong>${escapeHtml(message)}</strong></div>
  `;
  document.body.appendChild(toastEl);
  window.setTimeout(() => toastEl.remove(), 2600);
}

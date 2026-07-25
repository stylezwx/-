const SERIAL_KINDS = ["stm32", "esp32"];

const channelLabels = {
  stm32: "STM32",
  esp32: "ESP32",
};

const elements = {
  fanSpeedInput: document.getElementById("fanSpeedInput"),
  fanSpeedValue: document.getElementById("fanSpeedValue"),
  setFanSpeedButton: document.getElementById("setFanSpeedButton"),
  sendCustomButton: document.getElementById("sendCustomButton"),
  customCommandInput: document.getElementById("customCommandInput"),
  commandTargetSelect: document.getElementById("commandTargetSelect"),
  clearButton: null,
  logOutput: document.getElementById("logOutput"),
  lineCountLabel: document.getElementById("lineCountLabel"),
  temp1Value: document.getElementById("temp1Value"),
  temp2Value: document.getElementById("temp2Value"),
  countdownValue: document.getElementById("countdownValue"),
  alarmValue: document.getElementById("alarmValue"),
  dbEnabledInput: document.getElementById("dbEnabledInput"),
  autoSaveInput: document.getElementById("autoSaveInput"),
  dbHostInput: document.getElementById("dbHostInput"),
  dbPortInput: document.getElementById("dbPortInput"),
  dbUserInput: document.getElementById("dbUserInput"),
  dbPasswordInput: document.getElementById("dbPasswordInput"),
  dbNameInput: document.getElementById("dbNameInput"),
  dbTableInput: document.getElementById("dbTableInput"),
  saveDbConfigButton: document.getElementById("saveDbConfigButton"),
  initDbButton: document.getElementById("initDbButton"),
  dbStatusBadge: document.getElementById("dbStatusBadge"),
  dbRuntimeHint: document.getElementById("dbRuntimeHint"),
  quickCommandButtons: Array.from(document.querySelectorAll(".quick-command")),
};

const serialElements = {
  stm32: {
    connectButton: document.getElementById("stm32ConnectButton"),
    disconnectButton: document.getElementById("stm32DisconnectButton"),
    refreshPortsButton: document.getElementById("stm32RefreshPortsButton"),
    addPortButton: document.getElementById("stm32AddPortButton"),
    portSelect: document.getElementById("stm32PortSelect"),
    baudRateInput: document.getElementById("stm32BaudRateInput"),
    statusBadge: document.getElementById("stm32StatusBadge"),
  },
  esp32: {
    connectButton: document.getElementById("esp32ConnectButton"),
    disconnectButton: document.getElementById("esp32DisconnectButton"),
    refreshPortsButton: document.getElementById("esp32RefreshPortsButton"),
    addPortButton: document.getElementById("esp32AddPortButton"),
    portSelect: document.getElementById("esp32PortSelect"),
    baudRateInput: document.getElementById("esp32BaudRateInput"),
    statusBadge: document.getElementById("esp32StatusBadge"),
  },
};

const state = {
  availablePorts: [],
  dbConfig: null,
  saveQueue: Promise.resolve(),
  lineCount: 0,
  fanSpeedDraft: null,
  fanSpeedPendingAck: false,
  channels: {
    stm32: createChannelState(),
    esp32: createChannelState(),
  },
};

const telemetryPattern =
  /dht_1:\s*temp=([+-]?\d+(?:\.\d+)?)(?:\s*humi=([+-]?\d+(?:\.\d+)?))?\s*\|\s*dht_2:\s*temp=([+-]?\d+(?:\.\d+)?)(?:\s*humi=([+-]?\d+(?:\.\d+)?))?\s*\|\s*countdown=(\d+)s\s*\|\s*alarm=([+-]?\d+(?:\.\d+)?)(?:\s*\|\s*pwm=(\d+))?(?:\s*\|\s*pet=([a-z_]+))?(?:\s*\|\s*monitor=([a-z_]+))?/i;
const textEncoder = new TextEncoder();

void initialize();

function createChannelState() {
  return {
    port: null,
    reader: null,
    writer: null,
    writeQueue: Promise.resolve(),
    readableClosed: null,
    keepReading: false,
    lineBuffer: "",
  };
}

async function initialize() {
  wireEvents();
  syncFanSpeedDisplay();
  renderEmptyState();
  syncLineCount();
  SERIAL_KINDS.forEach((kind) => setConnectionState(kind, false));
  await loadDbConfig();

  if (!supportsSerial()) {
    appendSystemMessage("当前浏览器不支持 Web Serial API，请使用最新版本 Chrome 或 Edge。");
    disableSerialControls();
    return;
  }

  if (!window.isSecureContext) {
    appendSystemMessage("当前页面不是安全上下文，请通过 http://localhost 或 https 打开。");
  }

  navigator.serial.addEventListener("connect", handlePortListChange);
  navigator.serial.addEventListener("disconnect", handlePortDisconnect);
  await refreshPortOptions();
}

function wireEvents() {
  SERIAL_KINDS.forEach((kind) => {
    const ui = serialElements[kind];
    ui.connectButton.addEventListener("click", () => {
      void connectSerial(kind);
    });
    ui.disconnectButton.addEventListener("click", () => {
      void disconnectSerial(kind);
    });
    ui.refreshPortsButton.addEventListener("click", () => {
      void refreshPortOptions();
    });
    ui.addPortButton.addEventListener("click", () => {
      void requestNewPort(kind);
    });
    ui.portSelect.addEventListener("change", () => {
      syncPortAvailability(kind);
    });
  });

  elements.fanSpeedInput.addEventListener("input", handleFanSpeedInput);
  elements.setFanSpeedButton.addEventListener("click", () => {
    void sendFanSpeedCommand();
  });
  elements.sendCustomButton.addEventListener("click", () => {
    void sendCommand(elements.customCommandInput.value, elements.commandTargetSelect.value);
  });
  elements.customCommandInput.addEventListener("input", syncCommandAvailability);
  elements.commandTargetSelect.addEventListener("change", syncCommandAvailability);
  elements.customCommandInput.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !elements.sendCustomButton.disabled) {
      void sendCommand(elements.customCommandInput.value, elements.commandTargetSelect.value);
    }
  });
  elements.saveDbConfigButton.addEventListener("click", () => {
    void saveDbConfig();
  });
  elements.initDbButton.addEventListener("click", () => {
    void initializeDatabase();
  });
  elements.autoSaveInput.addEventListener("change", syncDbStatus);
  elements.dbEnabledInput.addEventListener("change", syncDbStatus);

  for (const button of elements.quickCommandButtons) {
    button.addEventListener("click", () => {
      void sendCommand(button.dataset.quickCommand ?? "", "stm32", { clearInput: false });
    });
  }
}

function supportsSerial() {
  return "serial" in navigator;
}

function canUseSerial() {
  return supportsSerial() && window.isSecureContext;
}

function disableSerialControls() {
  SERIAL_KINDS.forEach((kind) => {
    const ui = serialElements[kind];
    ui.connectButton.disabled = true;
    ui.disconnectButton.disabled = true;
    ui.refreshPortsButton.disabled = true;
    ui.addPortButton.disabled = true;
    ui.portSelect.disabled = true;
    ui.baudRateInput.disabled = true;
  });

  elements.customCommandInput.disabled = true;
  elements.commandTargetSelect.disabled = true;
  elements.sendCustomButton.disabled = true;
  elements.fanSpeedInput.disabled = true;
  elements.setFanSpeedButton.disabled = true;
  elements.quickCommandButtons.forEach((button) => {
    button.disabled = true;
  });
}

function getChannel(kind) {
  return state.channels[kind];
}

function getChannelLabel(kind) {
  return channelLabels[kind];
}

function getSerialUi(kind) {
  return serialElements[kind];
}

function getOtherKind(kind) {
  return kind === "stm32" ? "esp32" : "stm32";
}

function normalizeFanSpeed(value) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    return 0;
  }

  return Math.max(0, Math.min(100, Math.round(numericValue)));
}

function syncFanSpeedDisplay(input = elements.fanSpeedInput.value) {
  const value =
    input && typeof input === "object" && "target" in input
      ? input.target?.value
      : input;
  const duty = normalizeFanSpeed(value);
  elements.fanSpeedInput.value = String(duty);
  elements.fanSpeedValue.textContent = `${duty}%`;
}

function handleFanSpeedInput(event) {
  const duty = normalizeFanSpeed(event.target?.value);
  state.fanSpeedDraft = duty;
  state.fanSpeedPendingAck = false;
  syncFanSpeedDisplay(duty);
}

async function loadDbConfig() {
  try {
    const response = await fetch("/api/config");
    const payload = await readJsonResponse(response);
    state.dbConfig = payload.config;
    fillDbForm(payload.config);
    syncDbStatus(payload.statusMessage ?? "数据库配置已加载。");
  } catch (error) {
    elements.dbRuntimeHint.textContent = `数据库配置加载失败：${formatError(error)}`;
    appendSystemMessage(`数据库配置加载失败：${formatError(error)}`);
  }
}

function fillDbForm(config) {
  elements.dbEnabledInput.checked = Boolean(config.enabled);
  elements.autoSaveInput.checked = config.autoSave !== false;
  elements.dbHostInput.value = config.host ?? "127.0.0.1";
  elements.dbPortInput.value = String(config.port ?? 3306);
  elements.dbUserInput.value = config.user ?? "root";
  elements.dbPasswordInput.value = "";
  elements.dbPasswordInput.placeholder = config.passwordConfigured
    ? "已配置，留空保持不变"
    : "输入密码（不会写入配置文件）";
  elements.dbNameInput.value = config.database ?? "dht11_demo";
  elements.dbTableInput.value = config.table ?? "telemetry_log";
}

function collectDbForm() {
  return {
    enabled: elements.dbEnabledInput.checked,
    autoSave: elements.autoSaveInput.checked,
    host: elements.dbHostInput.value.trim(),
    port: Number(elements.dbPortInput.value),
    user: elements.dbUserInput.value.trim(),
    password: elements.dbPasswordInput.value,
    database: elements.dbNameInput.value.trim(),
    table: elements.dbTableInput.value.trim(),
  };
}

async function saveDbConfig() {
  const config = collectDbForm();

  try {
    setDbBusy(true, "正在保存数据库配置...");
    const response = await fetch("/api/config", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(config),
    });
    const payload = await readJsonResponse(response);
    state.dbConfig = payload.config;
    fillDbForm(payload.config);
    syncDbStatus("数据库配置已保存。");
    appendSystemMessage("数据库配置已保存。");
  } catch (error) {
    syncDbStatus(`保存失败：${formatError(error)}`);
    appendSystemMessage(`数据库配置保存失败：${formatError(error)}`);
  } finally {
    setDbBusy(false);
  }
}

async function initializeDatabase() {
  const config = collectDbForm();

  try {
    setDbBusy(true, "正在初始化数据库和数据表...");
    const response = await fetch("/api/init-db", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(config),
    });
    const payload = await readJsonResponse(response);
    state.dbConfig = payload.config;
    fillDbForm(payload.config);
    syncDbStatus(payload.message);
    appendSystemMessage(payload.message);
  } catch (error) {
    syncDbStatus(`初始化失败：${formatError(error)}`);
    appendSystemMessage(`初始化数据库失败：${formatError(error)}`);
  } finally {
    setDbBusy(false);
  }
}

async function refreshPortOptions(preferredPorts = {}) {
  if (!supportsSerial()) {
    return;
  }

  try {
    const preferred = {};
    SERIAL_KINDS.forEach((kind) => {
      preferred[kind] = preferredPorts[kind] ?? getChannel(kind).port ?? getSelectedPort(kind);
    });

    state.availablePorts = await navigator.serial.getPorts();
    SERIAL_KINDS.forEach((kind) => {
      renderPortOptions(kind, preferred[kind]);
    });
  } catch (error) {
    appendSystemMessage(`读取串口列表失败：${formatError(error)}`);
  }
}

function renderPortOptions(kind, preferredPort) {
  const ui = getSerialUi(kind);
  const activePort = getChannel(kind).port ?? preferredPort;
  ui.portSelect.innerHTML = "";

  if (state.availablePorts.length === 0) {
    const emptyOption = document.createElement("option");
    emptyOption.value = "";
    emptyOption.textContent = "暂无已授权串口";
    ui.portSelect.appendChild(emptyOption);
    ui.portSelect.value = "";
    syncPortAvailability(kind);
    return;
  }

  let selectedValue = "0";

  state.availablePorts.forEach((port, index) => {
    const option = document.createElement("option");
    option.value = String(index);
    option.textContent = formatPortLabel(port, index);
    if (port === activePort) {
      selectedValue = String(index);
    }
    ui.portSelect.appendChild(option);
  });

  ui.portSelect.value = selectedValue;
  syncPortAvailability(kind);
}

function formatPortLabel(port, index) {
  const info = port.getInfo();
  const pieces = [`串口 ${index + 1}`];

  if (typeof info.usbVendorId === "number") {
    pieces.push(`VID ${info.usbVendorId.toString(16).toUpperCase().padStart(4, "0")}`);
  }

  if (typeof info.usbProductId === "number") {
    pieces.push(`PID ${info.usbProductId.toString(16).toUpperCase().padStart(4, "0")}`);
  }

  const owners = SERIAL_KINDS
    .filter((kind) => getChannel(kind).port === port)
    .map((kind) => getChannelLabel(kind));
  if (owners.length > 0) {
    pieces.push(`${owners.join("/")} 已连接`);
  }

  if (pieces.length === 1) {
    pieces.push("已授权设备");
  }

  return pieces.join(" | ");
}

function getSelectedPort(kind) {
  const ui = getSerialUi(kind);
  const index = Number(ui.portSelect.value);
  if (!Number.isInteger(index) || index < 0) {
    return null;
  }

  return state.availablePorts[index] ?? null;
}

function syncPortAvailability(kind) {
  const ui = getSerialUi(kind);
  const channel = getChannel(kind);
  const isConnected = Boolean(channel.port);
  const hasPorts = state.availablePorts.length > 0;
  const hasSelectedPort = Boolean(getSelectedPort(kind));
  const serialReady = canUseSerial();

  ui.portSelect.disabled = isConnected || !hasPorts;
  ui.refreshPortsButton.disabled = !supportsSerial();
  ui.addPortButton.disabled = !serialReady;
  ui.connectButton.disabled = isConnected || !serialReady;
  ui.disconnectButton.disabled = !isConnected;
  ui.baudRateInput.disabled = isConnected;
  ui.connectButton.textContent = hasSelectedPort ? "连接串口" : "授权并连接串口";
}

function syncDbStatus(message = "") {
  const enabled = elements.dbEnabledInput.checked;
  const autoSave = elements.autoSaveInput.checked;
  const statusText = !enabled
    ? "未启用"
    : autoSave
      ? "自动写库中"
      : "仅保存配置";

  elements.dbStatusBadge.textContent = statusText;
  elements.dbStatusBadge.classList.toggle("online", enabled);
  elements.dbStatusBadge.classList.toggle("offline", !enabled);

  if (message) {
    elements.dbRuntimeHint.textContent = message;
    return;
  }

  if (!enabled) {
    elements.dbRuntimeHint.textContent = "启用后，解析出的 STM32 串口数据会通过本地服务写入 MySQL。";
  } else if (!autoSave) {
    elements.dbRuntimeHint.textContent = "数据库已启用，但当前关闭了自动保存。";
  } else {
    elements.dbRuntimeHint.textContent = "数据库已启用，新的 STM32 串口数据会自动入库。";
  }
}

function setDbBusy(isBusy, message = "") {
  elements.saveDbConfigButton.disabled = isBusy;
  elements.initDbButton.disabled = isBusy;

  if (message) {
    elements.dbRuntimeHint.textContent = message;
  }
}

async function requestNewPort(kind) {
  if (!canUseSerial()) {
    appendSystemMessage("当前页面不能访问串口，请确认使用 Chrome/Edge，并通过 http://localhost 或 https 打开。", getChannelLabel(kind));
    return;
  }

  try {
    const port = await navigator.serial.requestPort();
    await refreshPortOptions({ [kind]: port });
    appendSystemMessage(`已添加串口授权：${formatSelectedPortLabel(port)}。`, getChannelLabel(kind));
  } catch (error) {
    if (error && typeof error === "object" && error.name === "NotFoundError") {
      appendSystemMessage("你取消了串口授权。", getChannelLabel(kind));
      return;
    }

    appendSystemMessage(`新增串口失败：${formatError(error)}`, getChannelLabel(kind));
  }
}

async function connectSerial(kind) {
  const ui = getSerialUi(kind);
  const baudRate = Number(ui.baudRateInput.value);
  if (!Number.isInteger(baudRate) || baudRate <= 0) {
    appendSystemMessage("波特率必须是正整数。", getChannelLabel(kind));
    return;
  }

  if (!canUseSerial()) {
    appendSystemMessage("当前页面不能访问串口，请确认使用 Chrome/Edge，并通过 http://localhost 或 https 打开。", getChannelLabel(kind));
    return;
  }

  let port = getSelectedPort(kind);
  if (!port) {
    port = await requestPortForConnection(kind);
    if (!port) {
      return;
    }
  }

  const otherKind = getOtherKind(kind);
  if (getChannel(otherKind).port === port) {
    appendSystemMessage(`${getChannelLabel(otherKind)} 已连接到这个串口，请先断开后再切换。`, getChannelLabel(kind));
    return;
  }

  const channel = getChannel(kind);
  if (channel.port && channel.port !== port) {
    await safeClosePort(kind, { refresh: false });
  }

  try {
    await openPort(kind, port, baudRate);
    appendSystemMessage(
      `串口已连接：${formatSelectedPortLabel(port)}，参数 ${baudRate} / 8-N-1 / 无流控。`,
      getChannelLabel(kind),
    );
    await refreshPortOptions({ [kind]: port });
    void readLoop(kind, port);
  } catch (error) {
    appendSystemMessage(`连接失败：${formatConnectError(error)}`, getChannelLabel(kind));
    await safeClosePort(kind);
  }
}

async function requestPortForConnection(kind) {
  try {
    const port = await navigator.serial.requestPort();
    await refreshPortOptions({ [kind]: port });
    appendSystemMessage(`已授权并选中串口：${formatSelectedPortLabel(port)}。`, getChannelLabel(kind));
    return port;
  } catch (error) {
    if (error && typeof error === "object" && error.name === "NotFoundError") {
      appendSystemMessage("你取消了串口授权。", getChannelLabel(kind));
      return null;
    }

    appendSystemMessage(`申请串口权限失败：${formatError(error)}`, getChannelLabel(kind));
    return null;
  }
}

async function openPort(kind, port, baudRate) {
  await port.open({
    baudRate,
    dataBits: 8,
    stopBits: 1,
    parity: "none",
    flowControl: "none",
    bufferSize: 4096,
  });

  try {
    await port.setSignals({
      dataTerminalReady: true,
      requestToSend: true,
      break: false,
    });
  } catch (_) {
  }

  const textDecoder = new TextDecoderStream();
  const channel = getChannel(kind);

  channel.port = port;
  channel.readableClosed = port.readable.pipeTo(textDecoder.writable);
  channel.reader = textDecoder.readable.getReader();
  channel.writer = port.writable.getWriter();
  channel.writeQueue = Promise.resolve();
  channel.keepReading = true;
  channel.lineBuffer = "";
  setConnectionState(kind, true);
}

async function disconnectSerial(kind) {
  const portLabel = formatSelectedPortLabel(getChannel(kind).port);
  const wasConnected = Boolean(getChannel(kind).port || getChannel(kind).reader || getChannel(kind).writer);
  await safeClosePort(kind);

  if (wasConnected) {
    appendSystemMessage(`串口已断开：${portLabel}。`, getChannelLabel(kind));
  }
}

async function safeClosePort(kind, options = {}) {
  const channel = getChannel(kind);
  const reader = channel.reader;
  const writer = channel.writer;
  const readableClosed = channel.readableClosed;
  const port = channel.port;

  channel.keepReading = false;
  channel.port = null;
  channel.reader = null;
  channel.writer = null;
  channel.writeQueue = Promise.resolve();
  channel.readableClosed = null;
  channel.lineBuffer = "";

  try {
    if (reader) {
      await reader.cancel();
    }
  } catch (_) {
  }

  try {
    if (reader) {
      reader.releaseLock();
    }
  } catch (_) {
  }

  try {
    if (readableClosed) {
      await readableClosed.catch(() => {});
    }
  } catch (_) {
  }

  try {
    if (writer) {
      writer.releaseLock();
    }
  } catch (_) {
  }

  try {
    if (port) {
      await port.close();
    }
  } catch (_) {
  }

  setConnectionState(kind, false);

  if (options.refresh !== false) {
    await refreshPortOptions({ [kind]: options.preferredPort ?? port });
  }
}

async function readLoop(kind, activePort) {
  const channel = getChannel(kind);

  while (channel.reader && channel.keepReading && channel.port === activePort) {
    try {
      const { value, done } = await channel.reader.read();
      if (done) {
        break;
      }

      if (value) {
        consumeIncomingText(kind, value);
      }
    } catch (error) {
      if (channel.keepReading) {
        appendSystemMessage(`串口读取中断：${formatError(error)}`, getChannelLabel(kind));
      }
      break;
    }
  }

  if (getChannel(kind).port === activePort) {
    appendSystemMessage(`串口连接已结束：${formatSelectedPortLabel(activePort)}。`, getChannelLabel(kind));
    await safeClosePort(kind);
  }
}

function handlePortListChange() {
  void refreshPortOptions();
}

function handlePortDisconnect(event) {
  for (const kind of SERIAL_KINDS) {
    if (event.port === getChannel(kind).port) {
      appendSystemMessage("串口设备已断开，请重新连接。", getChannelLabel(kind));
      void safeClosePort(kind);
      return;
    }
  }

  void refreshPortOptions();
}

function consumeIncomingText(kind, chunk) {
  const channel = getChannel(kind);
  channel.lineBuffer += chunk;

  const parts = channel.lineBuffer.split(/\r?\n/);
  channel.lineBuffer = parts.pop() ?? "";

  for (const line of parts) {
    const normalized = line.trim();
    if (!normalized) {
      continue;
    }

    appendLogLine("RX", normalized, getChannelLabel(kind));
    if (kind === "stm32") {
      parseTelemetry(normalized);
    } else if (kind === "esp32") {
      void forwardEsp32ToStm32(normalized);
    }
  }
}

async function forwardEsp32ToStm32(message) {
  const stm32Channel = getChannel("stm32");
  if (!stm32Channel.writer) {
    return;
  }

  try {
    await writeToChannel("stm32", `${message}\r\n`);
    appendLogLine("TX", message, "ESP32->STM32");
  } catch (error) {
    appendSystemMessage(`ESP32 转发到 STM32 失败：${formatError(error)}`, "ESP32->STM32");
  }
}

function parseTelemetry(line) {
  const match = line.match(telemetryPattern);
  if (!match) {
    return;
  }

  const [, temp1, humi1, temp2, humi2, countdown, alarm, pwm, petStatus, petMonitor] = match;
  elements.temp1Value.textContent = temp1;
  elements.temp2Value.textContent = temp2;
  elements.countdownValue.textContent = countdown;
  elements.alarmValue.textContent = alarm;

  if (typeof pwm !== "undefined") {
    const actualPwm = normalizeFanSpeed(pwm);
    if (state.fanSpeedPendingAck && state.fanSpeedDraft === actualPwm) {
      state.fanSpeedPendingAck = false;
      state.fanSpeedDraft = null;
    }

    if (state.fanSpeedDraft === null) {
      syncFanSpeedDisplay(actualPwm);
    }
  }

  if (!elements.dbEnabledInput.checked || !elements.autoSaveInput.checked) {
    return;
  }

  const telemetry = {
    temp1: Number(temp1),
    humi1: Number(humi1 ?? 0),
    temp2: Number(temp2),
    humi2: Number(humi2 ?? 0),
    countdown: Number(countdown),
    alarm: Number(alarm),
    petStatus: petStatus ?? "unknown",
    petMonitor: petMonitor ?? "off",
    rawLine: line,
  };

  enqueueTelemetrySave(telemetry);
}

function enqueueTelemetrySave(telemetry) {
  state.saveQueue = state.saveQueue
    .then(() => postTelemetry(telemetry))
    .catch((error) => {
      syncDbStatus(`自动入库失败：${formatError(error)}`);
      appendSystemMessage(`自动入库失败：${formatError(error)}`);
    });
}

async function postTelemetry(telemetry) {
  const response = await fetch("/api/telemetry", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(telemetry),
  });
  const payload = await readJsonResponse(response);
  syncDbStatus(`最近写入成功：${new Date().toLocaleTimeString("zh-CN", { hour12: false })}`);
  return payload;
}

async function sendFanSpeedCommand() {
  const duty = normalizeFanSpeed(state.fanSpeedDraft ?? elements.fanSpeedInput.value);
  state.fanSpeedDraft = duty;
  state.fanSpeedPendingAck = true;
  syncFanSpeedDisplay(duty);
  await sendCommand(`pwm=${duty}`, "stm32", { clearInput: false });
}

async function sendCommand(rawText, kind = "stm32", options = {}) {
  const command = rawText.trim();
  if (!command) {
    return;
  }

  const channel = getChannel(kind);
  if (!channel.writer) {
    appendSystemMessage(`请先连接 ${getChannelLabel(kind)} 串口。`, getChannelLabel(kind));
    return;
  }

  try {
    await writeToChannel(kind, `${command}\r\n`);
    appendLogLine("TX", command, getChannelLabel(kind));
    if (options.clearInput !== false) {
      elements.customCommandInput.value = "";
      syncCommandAvailability();
    }
  } catch (error) {
    appendSystemMessage(`发送失败：${formatError(error)}`, getChannelLabel(kind));
  }
}

function writeToChannel(kind, text) {
  const channel = getChannel(kind);
  if (!channel.writer) {
    return Promise.reject(new Error(`${getChannelLabel(kind)} 串口未连接。`));
  }

  channel.writeQueue = channel.writeQueue.then(() => channel.writer.write(textEncoder.encode(text)));
  return channel.writeQueue;
}

function renderEmptyState() {
  if (elements.logOutput.childElementCount > 0) {
    return;
  }

  const empty = document.createElement("div");
  empty.className = "empty-state";
  empty.textContent = "等待串口数据...";
  elements.logOutput.appendChild(empty);
}

function appendSystemMessage(message, source = "APP") {
  appendLogLine("SYS", message, source);
}

function appendLogLine(direction, message, source = "APP") {
  const emptyState = elements.logOutput.querySelector(".empty-state");
  if (emptyState) {
    emptyState.remove();
  }

  const line = document.createElement("div");
  line.className = "log-line";

  const time = document.createElement("span");
  time.className = "log-time";
  time.textContent = new Date().toLocaleTimeString("zh-CN", { hour12: false });

  const dir = document.createElement("span");
  dir.className = `log-dir ${direction.toLowerCase()}`;
  dir.textContent = direction;

  const content = document.createElement("span");
  content.className = "log-message";
  content.textContent = `[${source}] ${message}`;

  line.append(time, dir, content);
  elements.logOutput.appendChild(line);
  elements.logOutput.scrollTop = elements.logOutput.scrollHeight;

  state.lineCount += 1;
  syncLineCount();
}

function syncLineCount() {
  elements.lineCountLabel.textContent = `${state.lineCount} 行`;
}

function syncCommandAvailability() {
  const targetKind = elements.commandTargetSelect.value || "stm32";
  const targetConnected = Boolean(getChannel(targetKind).writer);
  const anyConnected = SERIAL_KINDS.some((kind) => Boolean(getChannel(kind).writer));
  const stm32Connected = Boolean(getChannel("stm32").writer);

  elements.commandTargetSelect.disabled = !anyConnected;
  elements.customCommandInput.disabled = !anyConnected;
  elements.sendCustomButton.disabled = !targetConnected || !elements.customCommandInput.value.trim();
  elements.fanSpeedInput.disabled = !stm32Connected;
  elements.setFanSpeedButton.disabled = !stm32Connected;
  elements.quickCommandButtons.forEach((button) => {
    button.disabled = !stm32Connected;
  });
}

function setConnectionState(kind, isConnected) {
  const ui = getSerialUi(kind);
  ui.statusBadge.textContent = isConnected ? "已连接" : "未连接";
  ui.statusBadge.classList.toggle("online", isConnected);
  ui.statusBadge.classList.toggle("offline", !isConnected);
  syncPortAvailability(kind);
  syncCommandAvailability();
}

function formatSelectedPortLabel(port) {
  if (!port) {
    return "未选择串口";
  }

  const index = state.availablePorts.indexOf(port);
  return formatPortLabel(port, index >= 0 ? index : 0);
}

function formatConnectError(error) {
  if (error && typeof error === "object" && "name" in error) {
    switch (error.name) {
      case "InvalidStateError":
        return "串口状态异常，可能已经被当前页面或其他程序占用。";
      case "SecurityError":
        return "请通过 http://localhost 或 https 打开本页面。";
      case "NetworkError":
        return "串口打开失败。请确认 XCOM、串口助手等软件已经完全关闭。";
      case "NotFoundError":
        return "没有选择串口，或者你取消了授权。";
      default:
        break;
    }
  }

  return formatError(error);
}

async function readJsonResponse(response) {
  const payload = await response.json().catch(() => null);
  if (!response.ok) {
    const message = payload?.error ?? `请求失败：HTTP ${response.status}`;
    throw new Error(message);
  }

  return payload ?? {};
}

function formatError(error) {
  if (error instanceof Error) {
    return error.message;
  }

  return String(error);
}

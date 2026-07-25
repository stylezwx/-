"use strict";

const fs = require("node:fs");
const fsp = require("node:fs/promises");
const http = require("node:http");
const path = require("node:path");
const { execFile } = require("node:child_process");
const { promisify } = require("node:util");
const { URL } = require("node:url");

const execFileAsync = promisify(execFile);
const rootDir = __dirname;
const configPath = path.join(rootDir, "mysql.config.json");
const port = 8080;
let runtimePassword = process.env.SERIAL_WEB_MYSQL_PASSWORD ?? "";

const defaultConfig = {
  enabled: false,
  autoSave: true,
  host: "127.0.0.1",
  port: 3306,
  user: "root",
  password: "",
  database: "dht11_demo",
  table: "telemetry_log",
  mysqlBin: "mysql",
};

const mimeTypes = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".txt": "text/plain; charset=utf-8",
  ".svg": "image/svg+xml",
};

const server = http.createServer(async (request, response) => {
  try {
    const requestUrl = new URL(request.url, `http://${request.headers.host ?? "localhost"}`);

    if (requestUrl.pathname.startsWith("/api/")) {
      await handleApiRequest(request, response, requestUrl);
      return;
    }

    await serveStaticFile(request, response, requestUrl);
  } catch (error) {
    sendJson(response, 500, { error: formatError(error) });
  }
});

server.listen(port, "127.0.0.1", () => {
  console.log(`Serial web is running at http://localhost:${port}/`);
});

async function handleApiRequest(request, response, requestUrl) {
  if (request.method === "GET" && requestUrl.pathname === "/api/config") {
    const config = await loadConfig();
    sendJson(response, 200, {
      config: toPublicConfig(config),
      statusMessage: config.enabled
        ? "数据库配置已加载。"
        : "数据库未启用，保存配置后可开启自动写库。",
    });
    return;
  }

  if (request.method === "POST" && requestUrl.pathname === "/api/config") {
    const body = await readJsonBody(request);
    const config = normalizeConfigWithRuntimePassword(body);
    await saveConfig(config);
    sendJson(response, 200, { config: toPublicConfig(config) });
    return;
  }

  if (request.method === "POST" && requestUrl.pathname === "/api/init-db") {
    const body = await readJsonBody(request);
    const config = normalizeConfigWithRuntimePassword(body);
    await initializeDatabase(config);
    await saveConfig(config);
    sendJson(response, 200, {
      config: toPublicConfig(config),
      message: `数据库 ${config.database} 和数据表 ${config.table} 已初始化。`,
    });
    return;
  }

  if (request.method === "POST" && requestUrl.pathname === "/api/telemetry") {
    const telemetry = normalizeTelemetry(await readJsonBody(request));
    const config = await loadConfig();

    if (!config.enabled) {
      sendJson(response, 409, { error: "数据库写入未启用，请先保存并启用数据库配置。" });
      return;
    }

    await insertTelemetry(config, telemetry);
    sendJson(response, 200, { ok: true });
    return;
  }

  sendJson(response, 404, { error: "接口不存在。" });
}

async function serveStaticFile(request, response, requestUrl) {
  if (request.method !== "GET" && request.method !== "HEAD") {
    sendJson(response, 405, { error: "仅支持 GET 和 HEAD。" });
    return;
  }

  const targetPath = requestUrl.pathname === "/" ? "/index.html" : requestUrl.pathname;
  const decodedPath = decodeURIComponent(targetPath);
  const filePath = path.resolve(rootDir, `.${decodedPath}`);

  if (!filePath.startsWith(rootDir)) {
    sendJson(response, 403, { error: "禁止访问。" });
    return;
  }

  let stats;
  try {
    stats = await fsp.stat(filePath);
  } catch {
    sendJson(response, 404, { error: "文件不存在。" });
    return;
  }

  if (!stats.isFile()) {
    sendJson(response, 404, { error: "文件不存在。" });
    return;
  }

  const extension = path.extname(filePath).toLowerCase();
  response.statusCode = 200;
  response.setHeader("Content-Type", mimeTypes[extension] ?? "application/octet-stream");
  response.setHeader("Content-Length", stats.size);
  response.setHeader("Cache-Control", "no-store");

  if (request.method === "HEAD") {
    response.end();
    return;
  }

  fs.createReadStream(filePath).pipe(response);
}

async function loadConfig() {
  try {
    const content = await fsp.readFile(configPath, "utf8");
    return normalizeConfigWithRuntimePassword(JSON.parse(content));
  } catch (error) {
    if (error && error.code === "ENOENT") {
      return normalizeConfigWithRuntimePassword(defaultConfig);
    }

    throw error;
  }
}

async function saveConfig(config) {
  runtimePassword = config.password;
  const persistedConfig = { ...config };
  delete persistedConfig.password;
  await fsp.writeFile(configPath, `${JSON.stringify(persistedConfig, null, 2)}\n`, "utf8");
}

function normalizeConfigWithRuntimePassword(input) {
  const suppliedPassword = typeof input?.password === "string" ? input.password : "";
  return normalizeConfig({
    ...(input ?? {}),
    password: suppliedPassword || runtimePassword,
  });
}

function toPublicConfig(config) {
  return {
    ...config,
    password: "",
    passwordConfigured: Boolean(config.password),
  };
}

function normalizeConfig(input) {
  const merged = { ...defaultConfig, ...(input ?? {}) };

  return {
    enabled: Boolean(merged.enabled),
    autoSave: merged.autoSave !== false,
    host: normalizeString(merged.host, defaultConfig.host),
    port: normalizePort(merged.port),
    user: normalizeString(merged.user, defaultConfig.user),
    password: typeof merged.password === "string" ? merged.password : defaultConfig.password,
    database: normalizeIdentifier(merged.database, "数据库名"),
    table: normalizeIdentifier(merged.table, "数据表名"),
    mysqlBin: normalizeString(merged.mysqlBin, defaultConfig.mysqlBin),
  };
}

function normalizeTelemetry(input) {
  if (!input || typeof input !== "object") {
    throw new Error("遥测数据格式不正确。");
  }

  const telemetry = {
    temp1: normalizeNumber(input.temp1, "temp1"),
    humi1: normalizeNumber(input.humi1, "humi1"),
    temp2: normalizeNumber(input.temp2, "temp2"),
    humi2: normalizeNumber(input.humi2, "humi2"),
    countdown: normalizeInteger(input.countdown, "countdown"),
    alarm: normalizeNumber(input.alarm, "alarm"),
    petStatus: normalizeTelemetryLabel(input.petStatus, "petStatus"),
    petMonitor: normalizeTelemetryLabel(input.petMonitor, "petMonitor"),
    rawLine: normalizeRawLine(input.rawLine),
  };

  return telemetry;
}

async function initializeDatabase(config) {
  const tableName = quoteIdentifier(config.table);
  const createDatabaseSql = `CREATE DATABASE IF NOT EXISTS ${quoteIdentifier(config.database)} CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;`;
  const createTableSql = `
    CREATE TABLE IF NOT EXISTS ${tableName} (
      id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
      temp1 DECIMAL(6,2) NOT NULL,
      humi1 DECIMAL(6,2) NOT NULL,
      temp2 DECIMAL(6,2) NOT NULL,
      humi2 DECIMAL(6,2) NOT NULL,
      countdown_seconds INT NOT NULL,
      alarm_temp DECIMAL(6,2) NOT NULL,
      pet_status VARCHAR(32) NOT NULL DEFAULT 'unknown',
      pet_monitor VARCHAR(16) NOT NULL DEFAULT 'off',
      raw_line VARCHAR(255) NOT NULL,
      recorded_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
  `;

  await runMysqlCommand(config, createDatabaseSql);
  await runMysqlCommand(config, createTableSql, { database: config.database });
  await runMysqlCommand(config, buildEnsureColumnSql(config, "humi1", "DECIMAL(6,2) NOT NULL DEFAULT 0"), {
    database: config.database,
  });
  await runMysqlCommand(config, buildEnsureColumnSql(config, "humi2", "DECIMAL(6,2) NOT NULL DEFAULT 0"), {
    database: config.database,
  });
  await runMysqlCommand(config, buildEnsureColumnSql(config, "pet_status", "VARCHAR(32) NOT NULL DEFAULT 'unknown'"), {
    database: config.database,
  });
  await runMysqlCommand(config, buildEnsureColumnSql(config, "pet_monitor", "VARCHAR(16) NOT NULL DEFAULT 'off'"), {
    database: config.database,
  });
}

async function insertTelemetry(config, telemetry) {
  const sql = `
    INSERT INTO ${quoteIdentifier(config.table)} (
      temp1,
      humi1,
      temp2,
      humi2,
      countdown_seconds,
      alarm_temp,
      pet_status,
      pet_monitor,
      raw_line
    ) VALUES (
      ${telemetry.temp1.toFixed(2)},
      ${telemetry.humi1.toFixed(2)},
      ${telemetry.temp2.toFixed(2)},
      ${telemetry.humi2.toFixed(2)},
      ${telemetry.countdown},
      ${telemetry.alarm.toFixed(2)},
      ${quoteSqlString(telemetry.petStatus)},
      ${quoteSqlString(telemetry.petMonitor)},
      ${quoteSqlString(telemetry.rawLine)}
    );
  `;

  await runMysqlCommand(config, sql, { database: config.database });
}

function buildEnsureColumnSql(config, columnName, definition) {
  const alterSql = `ALTER TABLE ${quoteIdentifier(config.table)} ADD COLUMN ${quoteIdentifier(columnName)} ${definition}`;

  return `
    SET @column_exists = (
      SELECT COUNT(*)
      FROM INFORMATION_SCHEMA.COLUMNS
      WHERE TABLE_SCHEMA = ${quoteSqlString(config.database)}
        AND TABLE_NAME = ${quoteSqlString(config.table)}
        AND COLUMN_NAME = ${quoteSqlString(columnName)}
    );
    SET @ddl = IF(@column_exists = 0, ${quoteSqlString(alterSql)}, 'SELECT 1');
    PREPARE stmt FROM @ddl;
    EXECUTE stmt;
    DEALLOCATE PREPARE stmt;
  `;
}

async function runMysqlCommand(config, sql, options = {}) {
  const args = [
    "--protocol=TCP",
    "--default-character-set=utf8mb4",
    "-h",
    config.host,
    "-P",
    String(config.port),
    "-u",
    config.user,
  ];

  if (options.database) {
    args.push("-D", options.database);
  }

  args.push("-e", collapseSql(sql));

  try {
    await execFileAsync(config.mysqlBin, args, {
      windowsHide: true,
      env: {
        ...process.env,
        MYSQL_PWD: config.password,
      },
      timeout: 15000,
      maxBuffer: 1024 * 1024,
    });
  } catch (error) {
    const stderr = [error.stderr, error.stdout].filter(Boolean).join(" ").trim();
    throw new Error(stderr || formatError(error));
  }
}

function collapseSql(sql) {
  return sql
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean)
    .join(" ");
}

function normalizeString(value, fallback) {
  if (typeof value !== "string") {
    return fallback;
  }

  const trimmed = value.trim();
  return trimmed || fallback;
}

function normalizePort(value) {
  const portNumber = Number(value);
  if (!Number.isInteger(portNumber) || portNumber < 1 || portNumber > 65535) {
    throw new Error("端口必须是 1 到 65535 之间的整数。");
  }

  return portNumber;
}

function normalizeIdentifier(value, label) {
  const normalized = normalizeString(value, "");
  if (!/^[A-Za-z0-9_]+$/.test(normalized)) {
    throw new Error(`${label}只能包含字母、数字和下划线。`);
  }

  return normalized;
}

function normalizeNumber(value, label) {
  const numericValue = Number(value);
  if (!Number.isFinite(numericValue)) {
    throw new Error(`${label}必须是数字。`);
  }

  return numericValue;
}

function normalizeInteger(value, label) {
  const numericValue = Number(value);
  if (!Number.isInteger(numericValue)) {
    throw new Error(`${label}必须是整数。`);
  }

  return numericValue;
}

function normalizeTelemetryLabel(value, label) {
  if (typeof value !== "string") {
    throw new Error(`${label} must be a string.`);
  }

  const trimmed = value.trim().toLowerCase();
  if (!trimmed || !/^[a-z_]+$/.test(trimmed)) {
    throw new Error(`${label} has an invalid format.`);
  }

  return trimmed.slice(0, 32);
}

function normalizeRawLine(value) {
  if (typeof value !== "string") {
    throw new Error("rawLine 必须是字符串。");
  }

  const trimmed = value.trim();
  if (!trimmed) {
    throw new Error("rawLine 不能为空。");
  }

  return trimmed.slice(0, 255);
}

function quoteIdentifier(identifier) {
  return `\`${identifier}\``;
}

function quoteSqlString(value) {
  return `'${value.replace(/\\/g, "\\\\").replace(/'/g, "\\'")}'`;
}

async function readJsonBody(request) {
  const chunks = [];
  let size = 0;

  for await (const chunk of request) {
    size += chunk.length;
    if (size > 64 * 1024) {
      throw new Error("请求体过大。");
    }
    chunks.push(chunk);
  }

  if (chunks.length === 0) {
    return {};
  }

  const rawBody = Buffer.concat(chunks).toString("utf8");
  try {
    return JSON.parse(rawBody);
  } catch {
    throw new Error("请求体不是合法 JSON。");
  }
}

function sendJson(response, statusCode, payload) {
  const body = Buffer.from(JSON.stringify(payload), "utf8");
  response.statusCode = statusCode;
  response.setHeader("Content-Type", "application/json; charset=utf-8");
  response.setHeader("Content-Length", body.length);
  response.setHeader("Cache-Control", "no-store");
  response.end(body);
}

function formatError(error) {
  if (error instanceof Error) {
    return error.message;
  }

  return String(error);
}

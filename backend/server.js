import express from "express";
import cors from "cors";
import { SerialPort, ReadlineParser } from "serialport";

const HTTP_PORT = 3001;
const BAUD = 115200;
const PORT_HINT = process.env.SERIAL_PORT || null; // e.g. /dev/cu.usbmodem1101

let port = null;
let parser = null;
let portPath = null;
let lastError = null;
let positions = [90, 90, 90, 90, 90];

async function findArduinoPort() {
  if (PORT_HINT) return PORT_HINT;
  const ports = await SerialPort.list();
  // Pro Micro on macOS shows up as /dev/cu.usbmodem* — Arduino LLC vendor 0x2341,
  // SparkFun 0x1B4F, generic clones vary. Match on path first, then VID.
  const byPath = ports.find((p) => /usbmodem|usbserial/i.test(p.path));
  if (byPath) return byPath.path;
  const byVid = ports.find((p) =>
    ["2341", "1b4f", "239a", "16c0"].includes((p.vendorId || "").toLowerCase())
  );
  return byVid ? byVid.path : null;
}

async function openSerial() {
  if (port && port.isOpen) return;
  const path = await findArduinoPort();
  if (!path) {
    lastError = "no serial device found (plug in Pro Micro?)";
    portPath = null;
    return;
  }
  portPath = path;
  port = new SerialPort({ path, baudRate: BAUD }, (err) => {
    if (err) {
      lastError = err.message;
      port = null;
      console.error("serial open error:", err.message);
    } else {
      lastError = null;
      console.log("serial open:", path);
    }
  });
  parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));
  parser.on("data", (line) => {
    const s = line.trim();
    if (s.startsWith("P:")) {
      const vals = s.slice(2).split(",").map((n) => parseInt(n, 10));
      if (vals.length === 5 && vals.every((v) => !Number.isNaN(v))) {
        positions = vals;
      }
    }
    console.log("<-", s);
  });
  // probe: ask the sketch for its positions ~500ms after open
  setTimeout(() => {
    if (port && port.isOpen) {
      console.log("-> ? (probe)");
      port.write("?\n");
    }
  }, 500);
  port.on("close", () => {
    console.log("serial closed");
    port = null;
  });
  port.on("error", (e) => {
    lastError = e.message;
    console.error("serial err:", e.message);
  });
}

function send(line) {
  if (!port || !port.isOpen) return false;
  port.write(line + "\n");
  return true;
}

const app = express();
app.use(cors());
app.use(express.json());

app.get("/api/status", (req, res) => {
  res.json({
    connected: !!(port && port.isOpen),
    portPath,
    lastError,
    positions,
  });
});

app.post("/api/servo", (req, res) => {
  const { index, value } = req.body || {};
  const i = Number(index);
  let v = Number(value);
  if (!Number.isInteger(i) || i < 0 || i > 4) {
    return res.status(400).json({ error: "index must be 0..4" });
  }
  if (!Number.isFinite(v)) return res.status(400).json({ error: "bad value" });
  v = Math.max(0, Math.min(180, Math.round(v)));
  positions[i] = v;
  const ok = send(`S${i}:${v}`);
  res.json({ ok, index: i, value: v });
});

app.post("/api/center", (req, res) => {
  positions = [90, 90, 90, 90, 90];
  const ok = send("C");
  res.json({ ok, positions });
});

app.post("/api/reconnect", async (req, res) => {
  if (port && port.isOpen) port.close();
  port = null;
  await openSerial();
  res.json({ connected: !!(port && port.isOpen), portPath, lastError });
});

app.listen(HTTP_PORT, () => {
  console.log(`backend listening on http://localhost:${HTTP_PORT}`);
  openSerial();
});

// auto-reconnect: every 2s, try to open the port if we don't have one
setInterval(() => {
  if (!port || !port.isOpen) openSerial();
}, 2000);

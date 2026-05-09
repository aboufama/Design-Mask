import { useEffect, useRef, useState } from "react";

const N = 5;
const LABELS = ["01", "02", "03", "04", "05"];

export default function App() {
  const [values, setValues] = useState([90, 90, 90, 90, 90]);
  const [status, setStatus] = useState({ connected: false, portPath: null, lastError: null });
  const [reconnecting, setReconnecting] = useState(false);
  const sendTimers = useRef({});

  // poll status
  useEffect(() => {
    let alive = true;
    const tick = async () => {
      try {
        const r = await fetch("/api/status");
        const j = await r.json();
        if (alive) setStatus(j);
      } catch {
        if (alive) setStatus((s) => ({ ...s, connected: false }));
      }
    };
    tick();
    const id = setInterval(tick, 1500);
    return () => {
      alive = false;
      clearInterval(id);
    };
  }, []);

  const sendServo = (i, v) => {
    clearTimeout(sendTimers.current[i]);
    sendTimers.current[i] = setTimeout(() => {
      fetch("/api/servo", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ index: i, value: v }),
      }).catch(() => {});
    }, 20);
  };

  const setOne = (i, v) => {
    const clamped = Math.max(0, Math.min(180, Math.round(v)));
    setValues((prev) => {
      const next = [...prev];
      next[i] = clamped;
      return next;
    });
    sendServo(i, clamped);
  };

  const centerAll = async () => {
    setValues([90, 90, 90, 90, 90]);
    await fetch("/api/center", { method: "POST" }).catch(() => {});
  };

  const reconnect = async () => {
    setReconnecting(true);
    await fetch("/api/reconnect", { method: "POST" }).catch(() => {});
    setTimeout(() => setReconnecting(false), 600);
  };

  return (
    <div className="device">
      <header className="head">
        <div className="brand">designmask</div>
        <div className="model">5ch · linear servo</div>
        <div className={`link ${status.connected ? "ok" : "off"}`}>
          <span className="led" />
          {status.connected ? status.portPath || "link" : "no link"}
        </div>
      </header>

      <main className="panel">
        <div className="channels">
          {Array.from({ length: N }).map((_, i) => (
            <Channel
              key={i}
              label={LABELS[i]}
              value={values[i]}
              onChange={(v) => setOne(i, v)}
              onCenter={() => setOne(i, 90)}
            />
          ))}
        </div>

        <div className="masters">
          <BigButton onClick={centerAll} tone="orange">
            CENTER ALL
          </BigButton>
          <BigButton onClick={reconnect} tone="cream" loading={reconnecting}>
            {reconnecting ? "..." : "RECONNECT"}
          </BigButton>
        </div>
      </main>

      <footer className="foot">
        <span>v0.1</span>
        <span className="dots">· · · · ·</span>
        <span>{status.lastError ? `err: ${status.lastError}` : "ready"}</span>
      </footer>
    </div>
  );
}

function Channel({ label, value, onChange, onCenter }) {
  return (
    <div className="ch">
      <div className="ch-label">CH {label}</div>
      <div className="lcd">
        <span>{String(value).padStart(3, "0")}</span>
      </div>
      <Fader value={value} onChange={onChange} />
      <SmallButton onClick={onCenter}>CTR</SmallButton>
    </div>
  );
}

function Fader({ value, onChange }) {
  // vertical slider: 180 at top, 0 at bottom
  const trackRef = useRef(null);
  const pct = value / 180;

  const handle = (clientY) => {
    const r = trackRef.current.getBoundingClientRect();
    const t = 1 - (clientY - r.top) / r.height;
    onChange(Math.max(0, Math.min(1, t)) * 180);
  };

  const onPointerDown = (e) => {
    e.target.setPointerCapture(e.pointerId);
    handle(e.clientY);
  };
  const onPointerMove = (e) => {
    if (e.buttons === 0) return;
    handle(e.clientY);
  };

  const onKeyDown = (e) => {
    if (e.key === "ArrowUp") onChange(Math.min(180, value + (e.shiftKey ? 10 : 1)));
    else if (e.key === "ArrowDown") onChange(Math.max(0, value - (e.shiftKey ? 10 : 1)));
    else return;
    e.preventDefault();
  };

  return (
    <div
      className="fader"
      ref={trackRef}
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      tabIndex={0}
      onKeyDown={onKeyDown}
      role="slider"
      aria-valuemin={0}
      aria-valuemax={180}
      aria-valuenow={Math.round(value)}
    >
      <div className="fader-track">
        {Array.from({ length: 11 }).map((_, i) => (
          <span key={i} className="tick" style={{ bottom: `${(i / 10) * 100}%` }} />
        ))}
        <span className="mid" />
      </div>
      <div className="fader-knob" style={{ bottom: `calc(${pct * 100}% - 14px)` }}>
        <span className="grip" />
        <span className="grip" />
        <span className="grip" />
      </div>
    </div>
  );
}

function BigButton({ children, onClick, tone = "cream", loading }) {
  return (
    <button className={`btn big ${tone} ${loading ? "loading" : ""}`} onClick={onClick}>
      <span className="btn-face">{children}</span>
    </button>
  );
}

function SmallButton({ children, onClick }) {
  return (
    <button className="btn sm" onClick={onClick}>
      <span className="btn-face">{children}</span>
    </button>
  );
}

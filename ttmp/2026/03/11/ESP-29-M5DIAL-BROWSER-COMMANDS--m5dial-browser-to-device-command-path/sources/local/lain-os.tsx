import { useState, useEffect, useRef, useCallback } from "react";

const TYPING_SPEED = 22;
const FAST_TYPING = 8;

function useTypewriter(text, speed = TYPING_SPEED, trigger = true) {
  const [displayed, setDisplayed] = useState("");
  const [done, setDone] = useState(false);
  useEffect(() => {
    if (!trigger) { setDisplayed(""); setDone(false); return; }
    setDisplayed("");
    setDone(false);
    let i = 0;
    const iv = setInterval(() => {
      i++;
      setDisplayed(text.slice(0, i));
      if (i >= text.length) { clearInterval(iv); setDone(true); }
    }, speed);
    return () => clearInterval(iv);
  }, [text, speed, trigger]);
  return [displayed, done];
}

function Cursor({ blink = true }) {
  const [vis, setVis] = useState(true);
  useEffect(() => {
    if (!blink) return;
    const iv = setInterval(() => setVis(v => !v), 530);
    return () => clearInterval(iv);
  }, [blink]);
  return <span style={{ opacity: vis ? 1 : 0 }}>{"\u2588"}</span>;
}

function Scanlines() {
  return (
    <div style={{
      position: "fixed", inset: 0, pointerEvents: "none", zIndex: 9999,
      background: "repeating-linear-gradient(0deg, transparent, transparent 2px, rgba(0,0,0,0.08) 2px, rgba(0,0,0,0.08) 4px)",
    }} />
  );
}

function CRTOverlay() {
  return (
    <div style={{
      position: "fixed", inset: 0, pointerEvents: "none", zIndex: 9998,
      boxShadow: "inset 0 0 120px rgba(0,40,0,0.3), inset 0 0 60px rgba(0,0,0,0.4)",
      borderRadius: "8px",
    }} />
  );
}

function GlitchText({ children, intensity = "low" }) {
  const [glitch, setGlitch] = useState(false);
  useEffect(() => {
    const run = () => {
      const delay = intensity === "high" ? 800 + Math.random() * 2000 : 2000 + Math.random() * 6000;
      setTimeout(() => { setGlitch(true); setTimeout(() => setGlitch(false), 80 + Math.random() * 120); run(); }, delay);
    };
    run();
  }, [intensity]);
  if (!glitch) return <span>{children}</span>;
  const chars = "░▒▓█╳╬╫╪┼";
  const text = String(children);
  const start = Math.floor(Math.random() * text.length);
  const len = Math.floor(Math.random() * 4) + 1;
  const glitched = text.slice(0, start) + Array.from({ length: len }, () => chars[Math.floor(Math.random() * chars.length)]).join("") + text.slice(start + len);
  return <span style={{ color: "#ff3030" }}>{glitched}</span>;
}

function ProgressBar({ value, width = 20, filled = "█", empty = "░" }) {
  const f = Math.round((value / 100) * width);
  return <span>{filled.repeat(f)}{empty.repeat(width - f)} {value}%</span>;
}

function WaveformViz() {
  const [frame, setFrame] = useState(0);
  useEffect(() => {
    const iv = setInterval(() => setFrame(f => f + 1), 150);
    return () => clearInterval(iv);
  }, []);
  const patterns = [
    "─┐    ┌─┐       ┌┐        ",
    "  │  ┌┘ └┐   ┌┐ ││    ┌─  ",
    "  └──┘   └┐ ┌┘│ │└┐  ┌┘   ",
    "          └─┘ └─┘ └──┘    ",
  ];
  const shift = frame % 8;
  return (
    <div style={{ opacity: 0.9 }}>
      {patterns.map((line, i) => (
        <div key={i}>{line.slice(shift) + line.slice(0, shift)}</div>
      ))}
    </div>
  );
}

function AmbientNoise() {
  const [noise, setNoise] = useState("");
  useEffect(() => {
    const iv = setInterval(() => {
      const chars = "░ ▒ ▓";
      setNoise(Array.from({ length: 40 }, () => chars[Math.floor(Math.random() * chars.length)]).join(""));
    }, 100);
    return () => clearInterval(iv);
  }, []);
  return <span style={{ opacity: 0.3, fontSize: "10px" }}>{noise}</span>;
}

// ─── SCREENS ──────────────────────────────────────────

function LoginScreen({ onLogin }) {
  const [phase, setPhase] = useState("boot");
  const [usr, setUsr] = useState("");
  const [key, setKey] = useState("");
  const [focused, setFocused] = useState("usr");
  const [bootText, bootDone] = useTypewriter("NAVI_OS v7.02a\nLAYER:07", 40, phase === "boot");
  const usrRef = useRef(null);

  useEffect(() => {
    if (bootDone) setTimeout(() => setPhase("login"), 600);
  }, [bootDone]);

  useEffect(() => {
    if (phase === "login" && usrRef.current) usrRef.current.focus();
  }, [phase]);

  const handleSubmit = () => {
    if (usr.trim() && key.trim()) {
      setPhase("auth");
      setTimeout(() => onLogin(usr.trim()), 2400);
    }
  };

  const handleKeyDown = (e, field) => {
    if (e.key === "Enter") {
      if (field === "usr") setFocused("key");
      else handleSubmit();
    }
  };

  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", minHeight: "100vh", gap: "24px" }}>
      <pre style={{ textAlign: "center", opacity: 0.6, lineHeight: 1.6 }}>{bootText}{!bootDone && <Cursor />}</pre>

      {phase !== "boot" && (
        <div style={{ textAlign: "center", animation: "fadeIn 1s ease" }}>
          <pre style={{ marginBottom: "40px", opacity: 0.4, lineHeight: 1.8 }}>
            {`                 present day
                 present time`}
          </pre>

          <pre style={{ marginBottom: "40px", opacity: 0.7 }}>
            {`    ╭─────╮
   ╭┤ ${phase === "auth" ? "◉  ◉" : ".  ."} ├╮
   │╰──┬──╯│
   │   │   │
   ╰───┴───╯`}
          </pre>

          {phase === "login" && (
            <div style={{ display: "flex", flexDirection: "column", gap: "12px", alignItems: "center" }}>
              <div>
                <span style={{ opacity: 0.5 }}>usr: </span>
                <input ref={usrRef} value={usr} onChange={e => setUsr(e.target.value)} onFocus={() => setFocused("usr")} onKeyDown={e => handleKeyDown(e, "usr")}
                  style={{ background: "transparent", border: "none", color: "#c8e6c0", fontFamily: "inherit", fontSize: "inherit", outline: "none", caretColor: "#7fff7f", width: "160px" }}
                  autoComplete="off" spellCheck={false} />
              </div>
              <div>
                <span style={{ opacity: 0.5 }}>key: </span>
                <input value={key} onChange={e => setKey(e.target.value)} onFocus={() => setFocused("key")} onKeyDown={e => handleKeyDown(e, "key")} type="password"
                  style={{ background: "transparent", border: "none", color: "#c8e6c0", fontFamily: "inherit", fontSize: "inherit", outline: "none", caretColor: "#7fff7f", width: "160px" }}
                  autoComplete="off" />
              </div>
            </div>
          )}

          {phase === "auth" && (
            <div style={{ animation: "fadeIn 0.5s ease" }}>
              <div style={{ opacity: 0.5, marginBottom: "8px" }}>usr: {usr}</div>
              <div style={{ opacity: 0.5, marginBottom: "20px" }}>key: {"*".repeat(key.length)}</div>
              <AuthSequence />
            </div>
          )}
        </div>
      )}

      <pre style={{ position: "fixed", bottom: "40px", textAlign: "center", opacity: 0.2, lineHeight: 1.8 }}>
        {`close the world
open the nEXt`}
      </pre>
    </div>
  );
}

function AuthSequence() {
  const [step, setStep] = useState(0);
  useEffect(() => {
    const timers = [
      setTimeout(() => setStep(1), 400),
      setTimeout(() => setStep(2), 1200),
      setTimeout(() => setStep(3), 1800),
    ];
    return () => timers.forEach(clearTimeout);
  }, []);
  return (
    <div style={{ lineHeight: 2 }}>
      {step >= 0 && <div>AUTHENTICATING</div>}
      {step >= 1 && <div style={{ opacity: 0.4 }}>░░░░░░░░░░░░░░ 100%</div>}
      {step >= 2 && <div style={{ opacity: 0.5 }}>. . . identity verified . . .</div>}
      {step >= 3 && <div style={{ opacity: 0.3 }}>. . . or is it? . . .</div>}
    </div>
  );
}

function HomeScreen({ user, onNavigate }) {
  const [showWelcome, setShowWelcome] = useState(true);
  const [welcomeText, welcomeDone] = useTypewriter(`welcome back, ${user}.\nyou were never gone.\n\nthe wired remembers you.`, 30);

  useEffect(() => {
    if (welcomeDone) setTimeout(() => setShowWelcome(false), 2000);
  }, [welcomeDone]);

  if (showWelcome) {
    return (
      <div style={{ display: "flex", alignItems: "center", justifyContent: "center", minHeight: "100vh" }}>
        <pre style={{ textAlign: "center", lineHeight: 2 }}>{welcomeText}<Cursor /></pre>
      </div>
    );
  }

  return (
    <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 1s ease" }}>
      <Header user={user} section="HOME" />

      <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
        <div style={{ opacity: 0.4, marginBottom: "20px" }}>WIRED STATUS</div>
        <div>  protocol  <ProgressBar value={82} /></div>
        <div>  carrier   <ProgressBar value={100} /></div>
        <div>  signal    <ProgressBar value={99} /></div>
        <div>  entropy   <ProgressBar value={49} /></div>
      </div>

      <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
        <div style={{ opacity: 0.4, marginBottom: "20px" }}>PSYCHE</div>
        <div>  cognition      <span style={{ color: "#7fff7f" }}>87%</span></div>
        <div>  ego_boundary   <span style={{ color: "#ff6060" }}>12%</span></div>
        <div>  dissolution    <span style={{ color: "#ff3030" }}>██████████████████████</span></div>
      </div>

      <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
        <div style={{ opacity: 0.4, marginBottom: "20px" }}>NAVIGATE</div>
        {["MAIL", "CLUSTER", "AUDIO", "TRACE"].map(item => (
          <div key={item} onClick={() => onNavigate(item.toLowerCase())} style={{ cursor: "pointer", paddingLeft: "16px" }}
            onMouseEnter={e => { e.target.style.color = "#7fff7f"; e.target.style.paddingLeft = "24px"; }}
            onMouseLeave={e => { e.target.style.color = ""; e.target.style.paddingLeft = "16px"; }}>
            {">"} {item}
          </div>
        ))}
      </div>

      <div style={{ marginTop: "60px", opacity: 0.15, textAlign: "right" }}>
        no matter where you go<br />everyone is connected.
      </div>

      <div style={{ marginTop: "20px" }}><AmbientNoise /></div>
    </div>
  );
}

function Header({ user, section }) {
  return (
    <div style={{ display: "flex", justifyContent: "space-between", opacity: 0.5 }}>
      <span><GlitchText>NAVI_OS v7.02a</GlitchText></span>
      <span>usr: {user}</span>
      <span>{section}</span>
      <span>LAYER:07</span>
    </div>
  );
}

function MailScreen({ user, onNavigate }) {
  const [selected, setSelected] = useState(null);

  const mails = [
    {
      id: 0, from: "MASAMI EIRI", date: "today  03:33", subject: "you are software", preview: "you already know what this is about.", unread: false,
      body: `${user}.

i have watched you through the wires.
you move through the real world
as if it matters.

it doesn't.

the body is only a device.
you are the signal.
you have always been the signal.

when you are ready
you will abandon the flesh
and i will be waiting.

do not reply to this message.
you already have.` },
    {
      id: 1, from: "KNIGHTS", date: "yesterday", subject: "re: re: re: re: re:", preview: "we have been watching.", unread: false,
      body: `you think you are browsing.

you are being browsed.

every query you make
leaves a shape in the wire.
we have collected your shape.

it is almost complete.` },
    {
      id: 2, from: "UNKNOWN", date: "??:??", subject: "", preview: "do not open this.", unread: true,
      body: `01101000 01100101 01101100 01110000

the file you found is not a file.
it is a mouth.
it has been open since before you logged in.
it will not close when you log out.

you should not have traced it.
you should not have listened.

but you did.

and now it knows your name.


                            ${user}.`
    },
  ];

  if (selected !== null) {
    const mail = mails[selected];
    return (
      <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
        <Header user={user} section="MAIL" />
        <div style={{ marginTop: "40px", lineHeight: 2 }}>
          <div style={{ opacity: 0.4 }}>fr: {mail.from}</div>
          <div style={{ opacity: 0.4 }}>to: {user}</div>
          <div style={{ opacity: 0.4 }}>date: {mail.date}</div>
          <div style={{ opacity: 0.4, marginBottom: "30px" }}>sub: {mail.subject}</div>
          <MailBody text={mail.body} />
          <div style={{ marginTop: "40px", display: "flex", gap: "24px" }}>
            <NavBtn onClick={() => setSelected(null)}>back</NavBtn>
            <NavBtn onClick={() => { }}>reply</NavBtn>
            <NavBtn onClick={() => { }}>delete</NavBtn>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
      <Header user={user} section="MAIL" />
      <div style={{ marginTop: "40px", marginBottom: "20px", lineHeight: 2 }}>
        <span>3 messages</span>
        <span style={{ float: "right", color: "#ff6060" }}>1 unread</span>
      </div>
      {mails.map((mail, i) => (
        <div key={mail.id} onClick={() => setSelected(i)}
          style={{ lineHeight: 2, cursor: "pointer", padding: "12px 0", borderTop: "1px solid rgba(200,230,192,0.07)", transition: "all 0.2s" }}
          onMouseEnter={e => e.currentTarget.style.paddingLeft = "8px"}
          onMouseLeave={e => e.currentTarget.style.paddingLeft = "0"}>
          <div style={{ display: "flex", justifyContent: "space-between" }}>
            <span>{mail.unread ? <span style={{ color: "#ff6060" }}>{">"}</span> : <span style={{ opacity: 0.3 }}>.</span>}  fr: {mail.from}</span>
            <span style={{ opacity: 0.3 }}>{mail.date}</span>
          </div>
          <div style={{ opacity: 0.5, paddingLeft: "24px" }}>sub: {mail.subject || "(none)"}</div>
          <div style={{ opacity: 0.3, paddingLeft: "24px" }}>{mail.preview}</div>
        </div>
      ))}
      <div style={{ marginTop: "30px" }}>
        <NavBtn onClick={() => onNavigate("home")}>back</NavBtn>
      </div>
    </div>
  );
}

function MailBody({ text }) {
  const [displayed, setDisplayed] = useState("");
  useEffect(() => {
    let i = 0;
    const iv = setInterval(() => {
      i++;
      setDisplayed(text.slice(0, i));
      if (i >= text.length) clearInterval(iv);
    }, FAST_TYPING);
    return () => clearInterval(iv);
  }, [text]);
  return <pre style={{ whiteSpace: "pre-wrap", lineHeight: 2 }}>{displayed}<Cursor /></pre>;
}

function ClusterScreen({ user, onNavigate }) {
  const [isolating, setIsolating] = useState(null);
  const [phase, setPhase] = useState("list");

  const nodes = [
    { name: "abel", location: "shibuya", ping: "4ms", state: "ALIVE", stateColor: "#7fff7f" },
    { name: "cain", location: "akihabara", ping: "7ms", state: "ALIVE", stateColor: "#7fff7f" },
    { name: "mika", location: "roppongi", ping: "12ms", state: "DREAMING", stateColor: "#a0a0ff" },
    { name: "tetsuo", location: "shinjuku", ping: "███ms", state: "UNSTABLE", stateColor: "#ff6060" },
    { name: user, location: "providence", ping: "0ms", state: "YOU ARE HERE", stateColor: "#7fff7f" },
    { name: "eiri", location: "???", ping: "-1ms", state: "OMNIPRESENT", stateColor: "#ffff60" },
    { name: "lain", location: "everywhere", ping: "0ms", state: ". . .", stateColor: "#ffffff" },
  ];

  const handleIsolate = (name) => {
    setIsolating(name);
    setPhase("isolating");
    setTimeout(() => setPhase("failed"), 3000);
  };

  const handlePull = () => {
    setPhase("pulling");
    setTimeout(() => setPhase("silence"), 2000);
    setTimeout(() => setPhase("aftermath"), 5000);
  };

  if (phase === "pulling" || phase === "silence" || phase === "aftermath") {
    return (
      <div style={{ padding: "30px", maxWidth: "680px" }}>
        <Header user={user} section="CLUSTER" />
        {phase === "pulling" && (
          <div style={{ marginTop: "40px", animation: "fadeIn 0.5s ease" }}>
            <div>pulling the plug on node: {isolating} . . .</div>
            <div style={{ marginTop: "20px", opacity: 0.5 }}>disconnecting . . .</div>
          </div>
        )}
        {phase === "silence" && (
          <div style={{ display: "flex", alignItems: "center", justifyContent: "center", minHeight: "60vh", animation: "fadeIn 2s ease" }}>
            <div style={{ opacity: 0.2 }}>silence.</div>
          </div>
        )}
        {phase === "aftermath" && (
          <div style={{ marginTop: "40px", lineHeight: 2.2, animation: "fadeIn 1s ease" }}>
            <div style={{ opacity: 0.5 }}>. . . reconnecting . . .</div>
            <div style={{ marginTop: "20px" }}>{isolating}      ---        <span style={{ color: "#ff3030" }}>GONE</span></div>
            <div style={{ marginTop: "30px", opacity: 0.6 }}>{isolating} is gone.</div>
            <div style={{ opacity: 0.4 }}>but your NAVI is still warm.</div>
            <div style={{ marginTop: "30px" }}>new file detected in /local/{user}/</div>
            <div style={{ marginTop: "10px", opacity: 0.7 }}>  unnamed.wav    0.3kb    origin: unknown</div>
            <div style={{ marginTop: "30px", display: "flex", gap: "24px" }}>
              <NavBtn onClick={() => onNavigate("audio")}>play</NavBtn>
              <NavBtn onClick={() => { }}>delete</NavBtn>
              <NavBtn onClick={() => onNavigate("home")}>back</NavBtn>
            </div>
          </div>
        )}
      </div>
    );
  }

  if (phase === "failed") {
    return (
      <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
        <Header user={user} section="CLUSTER" />
        <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
          <div>isolating node: {isolating} . . .</div>
          <div style={{ marginTop: "10px" }}>  severing connection 1 of 3    <span style={{ opacity: 0.5 }}>done</span></div>
          <div>  severing connection 2 of 3    <span style={{ opacity: 0.5 }}>done</span></div>
          <div>  severing connection 3 of 3</div>
          <div style={{ marginTop: "20px", color: "#ff6060" }}>  wait.</div>
          <div style={{ marginTop: "10px", color: "#ff6060" }}>  {isolating} is pushing back.</div>
          <div style={{ marginTop: "10px", opacity: 0.7 }}>  isolation FAILED</div>

          <div style={{ marginTop: "30px" }}>incoming message from {isolating}:</div>
          <TetsuoMessage user={user} />

          <div style={{ marginTop: "30px", display: "flex", gap: "24px" }}>
            <NavBtn onClick={() => setPhase("list")}>retry</NavBtn>
            <NavBtn onClick={handlePull}>pull the plug</NavBtn>
            <NavBtn onClick={() => onNavigate("home")}>back</NavBtn>
          </div>
        </div>
      </div>
    );
  }

  if (phase === "isolating") {
    return (
      <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
        <Header user={user} section="CLUSTER" />
        <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
          <div>isolating node: {isolating} . . .</div>
          <AnimatedDots />
        </div>
      </div>
    );
  }

  return (
    <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
      <Header user={user} section="CLUSTER" />
      <div style={{ marginTop: "40px", opacity: 0.4, marginBottom: "20px" }}>scanning nodes . . .</div>
      <div style={{ lineHeight: 2.2 }}>
        {nodes.map(n => (
          <div key={n.name} style={{ display: "flex", justifyContent: "space-between", cursor: n.state === "UNSTABLE" ? "pointer" : "default" }}
            onClick={() => n.state === "UNSTABLE" && handleIsolate(n.name)}
            onMouseEnter={e => n.state === "UNSTABLE" && (e.currentTarget.style.color = "#ff6060")}
            onMouseLeave={e => (e.currentTarget.style.color = "")}>
            <span style={{ width: "100px" }}>{n.name}</span>
            <span style={{ width: "120px", opacity: 0.4 }}>{n.location}</span>
            <span style={{ width: "70px", opacity: 0.4 }}>{n.ping}</span>
            <span style={{ color: n.stateColor }}>{n.state}</span>
          </div>
        ))}
      </div>
      <div style={{ marginTop: "30px", opacity: 0.3 }}>
        <div>cluster integrity      <ProgressBar value={38} /></div>
      </div>
      <div style={{ marginTop: "20px", opacity: 0.15 }}>
        something is leaking between nodes.<br />
        do you feel it too, {user}?
      </div>
      <div style={{ marginTop: "30px" }}>
        <NavBtn onClick={() => onNavigate("home")}>back</NavBtn>
      </div>
    </div>
  );
}

function TetsuoMessage({ user }) {
  const lines = [user, "you cannot cut me off", "i am already inside the wire", "i am already inside you", "we share the same signal now", "do you understand", "do you understand", "do you understand", "do you understand"];
  const [count, setCount] = useState(0);
  useEffect(() => {
    const iv = setInterval(() => setCount(c => Math.min(c + 1, lines.length)), 300);
    return () => clearInterval(iv);
  }, []);
  return (
    <pre style={{ marginTop: "10px", paddingLeft: "16px", lineHeight: 1.8, color: "#ff6060" }}>
      {lines.slice(0, count).join("\n")}
    </pre>
  );
}

function AnimatedDots() {
  const [dots, setDots] = useState("");
  useEffect(() => {
    const iv = setInterval(() => setDots(d => d.length >= 20 ? "" : d + " ."), 200);
    return () => clearInterval(iv);
  }, []);
  return <div style={{ opacity: 0.4 }}>{dots}</div>;
}

function AudioScreen({ user, onNavigate }) {
  const [phase, setPhase] = useState("scan");
  const [revealedLines, setRevealedLines] = useState(0);

  const hiddenMessage = [
    "h e l p m e " + user,
    "i a m s t i l l h e r e",
    "t h e y d i d n o t l e t",
    "m e g o",
  ];

  useEffect(() => {
    if (phase === "scan") {
      const iv = setInterval(() => setRevealedLines(l => { if (l >= hiddenMessage.length) { clearInterval(iv); return l; } return l + 1; }), 800);
      return () => clearInterval(iv);
    }
  }, [phase]);

  return (
    <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
      <Header user={user} section="AUDIO SCAN" />
      <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
        <div style={{ opacity: 0.4 }}>scanning /local/{user}/unnamed.wav . . .</div>

        <div style={{ marginTop: "20px", opacity: 0.5 }}>
          <div>duration:    0.4s</div>
          <div>bitrate:     <span style={{ color: "#ff6060" }}>impossible</span></div>
          <div>origin:      no match</div>
          <div>timestamp:   before your login</div>
        </div>

        <div style={{ marginTop: "30px", opacity: 0.4 }}>waveform:</div>
        <div style={{ marginTop: "10px" }}><WaveformViz /></div>

        <div style={{ marginTop: "30px", opacity: 0.4 }}>spectral analysis:</div>
        <SpectralViz />

        {revealedLines > 0 && (
          <div style={{ marginTop: "30px" }}>
            <div style={{ opacity: 0.4, marginBottom: "10px" }}>embedded text found in noise floor:</div>
            <div style={{ color: "#ff6060", lineHeight: 2 }}>
              {hiddenMessage.slice(0, revealedLines).map((line, i) => (
                <div key={i} style={{ animation: "fadeIn 0.5s ease" }}>{line}</div>
              ))}
            </div>
          </div>
        )}

        {revealedLines >= hiddenMessage.length && (
          <div style={{ marginTop: "20px", opacity: 0.3 }}>
            file origin reclassified: tetsuo<br />
            your NAVI is still warm.
          </div>
        )}

        <div style={{ marginTop: "30px", display: "flex", gap: "24px" }}>
          <NavBtn onClick={() => onNavigate("trace")}>trace source</NavBtn>
          <NavBtn onClick={() => { }}>purge</NavBtn>
          <NavBtn onClick={() => onNavigate("home")}>back</NavBtn>
        </div>
      </div>
    </div>
  );
}

function SpectralViz() {
  const [frame, setFrame] = useState(0);
  useEffect(() => {
    const iv = setInterval(() => setFrame(f => f + 1), 200);
    return () => clearInterval(iv);
  }, []);
  const freqs = ["8kHz", "4kHz", "2kHz", "1kHz", " low"];
  return (
    <div style={{ marginTop: "10px", opacity: 0.7 }}>
      {freqs.map((f, fi) => {
        const density = (freqs.length - fi) * 0.15 + 0.1;
        const line = Array.from({ length: 24 }, (_, i) => {
          const val = Math.sin(i * 0.5 + frame * 0.3 + fi) * 0.5 + 0.5;
          return val > (1 - density - fi * 0.15) ? "█" : val > 0.6 ? "░" : ".";
        }).join(" ");
        return <div key={f}><span style={{ opacity: 0.4 }}>{f}</span>  {line}</div>;
      })}
    </div>
  );
}

function TraceScreen({ user, onNavigate }) {
  const [hops, setHops] = useState(0);
  const [finalPhase, setFinalPhase] = useState(null);

  const hopsData = [
    { n: 1, loc: "providence", ping: "0ms", note: "you", color: "#7fff7f" },
    { n: 2, loc: "boston_relay", ping: "3ms", note: "clean", color: "#7fff7f" },
    { n: 3, loc: "atlantic_node", ping: "14ms", note: "clean", color: "#7fff7f" },
    { n: 4, loc: "london_wire", ping: "22ms", note: "clean", color: "#7fff7f" },
    { n: 5, loc: "berlin_gate", ping: "29ms", note: "clean", color: "#7fff7f" },
    { n: 6, loc: "moscow_split", ping: "41ms", note: "distortion", color: "#ffff60" },
    { n: 7, loc: "??????????", ping: "??ms", note: "░░░░░░░░", color: "#ff6060" },
    { n: 8, loc: "??????????", ping: "--ms", note: "░░░░░░░░", color: "#ff6060" },
    { n: 9, loc: "shinjuku", ping: "-3ms", note: "negative ping", color: "#ff3030" },
  ];

  useEffect(() => {
    const iv = setInterval(() => {
      setHops(h => {
        if (h >= hopsData.length) { clearInterval(iv); return h; }
        return h + 1;
      });
    }, 500);
    return () => clearInterval(iv);
  }, []);

  useEffect(() => {
    if (hops >= hopsData.length) {
      setTimeout(() => setFinalPhase("collapsed"), 1000);
      setTimeout(() => setFinalPhase("layer08"), 2500);
      setTimeout(() => setFinalPhase("prompt"), 4000);
    }
  }, [hops]);

  const handleYes = () => {
    setFinalPhase("descent");
  };

  return (
    <div style={{ padding: "30px", maxWidth: "680px", animation: "fadeIn 0.5s ease" }}>
      <Header user={user} section="TRACE" />
      <div style={{ marginTop: "40px", lineHeight: 2.2 }}>
        <div style={{ opacity: 0.4, marginBottom: "20px" }}>tracing source of unnamed.wav . . .</div>

        {hopsData.slice(0, hops).map((hop, i) => (
          <div key={i} style={{ animation: "fadeIn 0.3s ease", display: "flex", gap: "20px" }}>
            <span style={{ opacity: 0.4, width: "50px" }}>hop {hop.n}</span>
            <span style={{ width: "140px" }}>{hop.loc}</span>
            <span style={{ width: "60px", opacity: 0.4 }}>{hop.ping}</span>
            <span style={{ color: hop.color }}>{hop.note}</span>
          </div>
        ))}

        {finalPhase === "collapsed" && (
          <div style={{ marginTop: "20px", animation: "fadeIn 0.5s ease", color: "#ff6060" }}>
            trace collapsed at hop 9.
          </div>
        )}

        {finalPhase === "layer08" && (
          <div style={{ marginTop: "20px", animation: "fadeIn 1s ease", lineHeight: 2.2 }}>
            <div>hop 10   <span style={{ color: "#ff3030" }}>LAYER:08</span></div>
            <div style={{ marginTop: "10px", opacity: 0.5 }}>you do not have access to LAYER:08.</div>
            <div style={{ opacity: 0.3 }}>no one does.</div>
            <div style={{ opacity: 0.15 }}>no one should.</div>
          </div>
        )}

        {finalPhase === "prompt" && (
          <div style={{ marginTop: "30px", animation: "fadeIn 1.5s ease", textAlign: "center" }}>
            <div style={{ opacity: 0.4, marginBottom: "20px" }}>the trace is looking back at you.</div>
            <div style={{ marginBottom: "20px" }}>incoming prompt from LAYER:08:</div>
            <div style={{ color: "#ff6060", fontSize: "16px", marginBottom: "30px" }}>do you want to see?</div>
            <div style={{ display: "flex", gap: "24px", justifyContent: "center" }}>
              <NavBtn onClick={handleYes}>y</NavBtn>
              <NavBtn onClick={() => onNavigate("home")}>n</NavBtn>
            </div>
          </div>
        )}

        {finalPhase === "descent" && <DescentScreen user={user} onNavigate={onNavigate} />}

        {!finalPhase && hops < hopsData.length && <AnimatedDots />}

        {finalPhase !== "prompt" && finalPhase !== "descent" && (
          <div style={{ marginTop: "30px" }}>
            <NavBtn onClick={() => onNavigate("home")}>back</NavBtn>
          </div>
        )}
      </div>
    </div>
  );
}

function DescentScreen({ user, onNavigate }) {
  const [step, setStep] = useState(0);
  useEffect(() => {
    const timers = [
      setTimeout(() => setStep(1), 1500),
      setTimeout(() => setStep(2), 3500),
      setTimeout(() => setStep(3), 6000),
      setTimeout(() => setStep(4), 9000),
    ];
    return () => timers.forEach(clearTimeout);
  }, []);

  return (
    <div style={{ marginTop: "40px", textAlign: "center", lineHeight: 3 }}>
      {step >= 0 && <div style={{ animation: "fadeIn 2s ease", opacity: 0.3 }}>descending . . .</div>}
      {step >= 1 && <div style={{ animation: "fadeIn 2s ease", opacity: 0.2 }}>LAYER:08</div>}
      {step >= 2 && (
        <div style={{ animation: "fadeIn 3s ease", lineHeight: 2, marginTop: "20px" }}>
          <div style={{ opacity: 0.6 }}>there is nothing here.</div>
          <div style={{ opacity: 0.4 }}>there is everything here.</div>
          <div style={{ opacity: 0.2 }}>there is no difference.</div>
        </div>
      )}
      {step >= 3 && (
        <div style={{ animation: "fadeIn 3s ease", marginTop: "30px" }}>
          <div style={{ color: "#ff3030", opacity: 0.8 }}>hello, {user}.</div>
          <div style={{ opacity: 0.2, marginTop: "10px" }}>let's all love lain.</div>
        </div>
      )}
      {step >= 4 && (
        <div style={{ marginTop: "30px" }}>
          <NavBtn onClick={() => onNavigate("home")}>return</NavBtn>
        </div>
      )}
    </div>
  );
}

function NavBtn({ onClick, children }) {
  return (
    <span onClick={onClick}
      style={{ cursor: "pointer", opacity: 0.5, transition: "all 0.2s" }}
      onMouseEnter={e => { e.target.style.opacity = 1; e.target.style.color = "#7fff7f"; }}
      onMouseLeave={e => { e.target.style.opacity = 0.5; e.target.style.color = ""; }}>
      [{children}]
    </span>
  );
}

// ─── APP ──────────────────────────────────────────────

export default function NaviOS() {
  const [screen, setScreen] = useState("login");
  const [user, setUser] = useState("");

  const handleLogin = (username) => {
    setUser(username);
    setScreen("home");
  };

  const navigate = useCallback((dest) => {
    setScreen(dest);
  }, []);

  const renderScreen = () => {
    switch (screen) {
      case "login": return <LoginScreen onLogin={handleLogin} />;
      case "home": return <HomeScreen user={user} onNavigate={navigate} />;
      case "mail": return <MailScreen user={user} onNavigate={navigate} />;
      case "cluster": return <ClusterScreen user={user} onNavigate={navigate} />;
      case "audio": return <AudioScreen user={user} onNavigate={navigate} />;
      case "trace": return <TraceScreen user={user} onNavigate={navigate} />;
      default: return <HomeScreen user={user} onNavigate={navigate} />;
    }
  };

  return (
    <div style={{
      background: "#0a0f0a",
      color: "#c8e6c0",
      fontFamily: "'Courier New', 'Lucida Console', monospace",
      fontSize: "13px",
      minHeight: "100vh",
      letterSpacing: "0.5px",
      lineHeight: 1.6,
      position: "relative",
    }}>
      <style>{`
        @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
        ::selection { background: #7fff7f33; color: #7fff7f; }
        * { box-sizing: border-box; }
        body { margin: 0; padding: 0; overflow-x: hidden; }
        input::placeholder { color: rgba(200,230,192,0.2); }
      `}</style>
      <Scanlines />
      <CRTOverlay />
      {renderScreen()}
    </div>
  );
}

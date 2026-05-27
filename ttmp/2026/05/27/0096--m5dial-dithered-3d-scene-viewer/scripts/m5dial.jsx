import { useEffect, useRef, useState } from 'react';
import * as THREE from 'three';

/* ============================================================
   M5 DIAL SIMULATOR
   Real 3D scenes rendered at low resolution and quantized to a
   4-color palette via Bayer ordered dithering. Scroll the dial
   (or drag) to orbit the camera.
   ============================================================ */

const SCENES = [
  { id: 'terrain', name: 'TERRAIN', subtitle: 'alp.001', glyph: '△' },
  { id: 'torus',   name: 'TOROID',  subtitle: 'geo.002', glyph: '◯' },
  { id: 'ocean',   name: 'OCEAN',   subtitle: 'tide.003', glyph: '≈' },
  { id: 'planet',  name: 'PLANET',  subtitle: 'sat.004', glyph: '◐' },
  { id: 'tunnel',  name: 'TUNNEL',  subtitle: 'tube.005', glyph: '◫' },
];

const PALETTES = {
  classic:  { warm: '#ff2940', cool: '#3050ff', high: '#ffffff', label: 'CLASSIC' },
  inverted: { warm: '#3050ff', cool: '#ff2940', high: '#ffffff', label: 'INVERTED' },
  red:      { warm: '#ff2940', cool: '#7a1020', high: '#ffffff', label: 'RED MONO' },
  blue:     { warm: '#5a78ff', cool: '#3050ff', high: '#ffffff', label: 'BLUE MONO' },
  amber:    { warm: '#ffae20', cool: '#5a3010', high: '#ffe080', label: 'AMBER CRT' },
};

/* ----------------------------------------------------------
   Dither shader
   Renders the low-res scene into a 4-color quantized image
   using a 4x4 Bayer threshold matrix.
   ---------------------------------------------------------- */

const DITHER_VS = `
varying vec2 vUv;
void main() {
  vUv = uv;
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}
`;

const DITHER_FS = `
varying vec2 vUv;
uniform sampler2D tDiffuse;
uniform vec2 uResolution;
uniform float uPixelSize;
uniform vec3 uColorWarm;
uniform vec3 uColorCool;
uniform vec3 uColorHigh;
uniform float uMaskRadius;
uniform float uContrast;

float bayer4x4(vec2 p) {
  int x = int(mod(p.x, 4.0));
  int y = int(mod(p.y, 4.0));
  int i = y * 4 + x;

  float v = 0.0;
  if (i == 0) v = 0.0;
  else if (i == 1) v = 8.0;
  else if (i == 2) v = 2.0;
  else if (i == 3) v = 10.0;
  else if (i == 4) v = 12.0;
  else if (i == 5) v = 4.0;
  else if (i == 6) v = 14.0;
  else if (i == 7) v = 6.0;
  else if (i == 8) v = 3.0;
  else if (i == 9) v = 11.0;
  else if (i == 10) v = 1.0;
  else if (i == 11) v = 9.0;
  else if (i == 12) v = 15.0;
  else if (i == 13) v = 7.0;
  else if (i == 14) v = 13.0;
  else v = 5.0;

  return v / 16.0;
}

void main() {
  // Pixelate
  vec2 pix = floor(vUv * uResolution / uPixelSize);
  vec2 sUv = (pix * uPixelSize + uPixelSize * 0.5) / uResolution;
  vec3 c = texture2D(tDiffuse, sUv).rgb;

  // Apply user contrast (S-curve-ish)
  c = clamp((c - 0.5) * uContrast + 0.5, 0.0, 1.0);

  // Bayer threshold for this pixel block
  float t = bayer4x4(pix);

  vec3 outColor = vec3(0.0);

  bool isWhite = c.r > 0.55 && c.g > 0.55 && c.b > 0.55;
  float lum = (c.r + c.g + c.b) / 3.0;

  if (isWhite && lum > t) {
    outColor = uColorHigh;
  } else if (c.r > c.b + 0.05) {
    // Warm-dominant
    if (c.r > t) outColor = uColorWarm;
  } else if (c.b > c.r + 0.05) {
    // Cool-dominant
    if (c.b > t) outColor = uColorCool;
  } else if (lum > 0.25 && lum > t) {
    // Neutral but bright - lean cool
    outColor = uColorCool;
  }

  // Circular mask
  vec2 d = vUv - 0.5;
  float dist = length(d) * 2.0;
  if (dist > uMaskRadius) outColor = vec3(0.0);

  gl_FragColor = vec4(outColor, 1.0);
}
`;

/* ----------------------------------------------------------
   Scene factories — each returns { group, update, distance, height, target }
   Materials use MeshLambertMaterial or vertex colors so that
   lighting + shading gradients produce dither density variation.
   ---------------------------------------------------------- */

function noise2d(x, y) {
  return (
    Math.sin(x * 0.45 + 1.2) * Math.cos(y * 0.31 + 0.4) * 0.55 +
    Math.sin(x * 1.1 + 0.5) * Math.cos(y * 0.78 + 1.3) * 0.30 +
    Math.sin(x * 2.3 + 2.1) * Math.cos(y * 1.7 + 0.2) * 0.15
  );
}

function buildTerrain() {
  const group = new THREE.Group();

  // Terrain
  const geo = new THREE.PlaneGeometry(40, 40, 80, 80);
  const positions = geo.attributes.position;
  const colors = new Float32Array(positions.count * 3);

  for (let i = 0; i < positions.count; i++) {
    const x = positions.getX(i);
    const y = positions.getY(i);
    let h = noise2d(x * 0.4, y * 0.4) * 4.5;
    // Push down in middle valley
    const distC = Math.sqrt(x * x + y * y);
    h += (1.0 - Math.min(1.0, distC / 8)) * -1.0;
    positions.setZ(i, h);

    const t = Math.max(0, Math.min(1, (h + 2) / 6));
    // Dark blue valleys → bright blue peaks
    colors[i * 3 + 0] = 0.04 + t * 0.15;
    colors[i * 3 + 1] = 0.06 + t * 0.20;
    colors[i * 3 + 2] = 0.25 + t * 0.65;
  }
  geo.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  geo.computeVertexNormals();
  geo.rotateX(-Math.PI / 2);

  const mat = new THREE.MeshBasicMaterial({ vertexColors: true });
  group.add(new THREE.Mesh(geo, mat));

  // Sun
  const sun = new THREE.Mesh(
    new THREE.SphereGeometry(1.2, 32, 32),
    new THREE.MeshBasicMaterial({ color: 0xff2030 })
  );
  sun.position.set(0, 4.5, -8);
  group.add(sun);

  // Sun halo (slight glow with another sphere)
  const halo = new THREE.Mesh(
    new THREE.SphereGeometry(1.45, 32, 32),
    new THREE.MeshBasicMaterial({ color: 0x600810, transparent: true, opacity: 0.7 })
  );
  halo.position.copy(sun.position);
  group.add(halo);

  return {
    group,
    update: (t) => {
      sun.position.y = 4.5 + Math.sin(t * 0.3) * 0.4;
      halo.position.copy(sun.position);
    },
    distance: 11,
    height: 3.2,
    target: new THREE.Vector3(0, 1.5, 0),
  };
}

function buildTorus() {
  const group = new THREE.Group();

  const geo = new THREE.TorusKnotGeometry(2.4, 0.85, 180, 24, 2, 3);
  const positions = geo.attributes.position;
  const colors = new Float32Array(positions.count * 3);

  for (let i = 0; i < positions.count; i++) {
    const x = positions.getX(i);
    const y = positions.getY(i);
    const z = positions.getZ(i);
    // Color based on position — red on one axis, blue on the other
    const a = (x + 3) / 6;
    const b = (y + 3) / 6;
    colors[i * 3 + 0] = Math.max(0, Math.min(1, a)) * 1.0;
    colors[i * 3 + 1] = 0;
    colors[i * 3 + 2] = Math.max(0, Math.min(1, 1 - a)) * 1.0;
    // brightness modulated by z
    const lit = 0.4 + Math.max(0, z / 4) * 0.6;
    colors[i * 3 + 0] *= lit;
    colors[i * 3 + 2] *= lit;
  }
  geo.setAttribute('color', new THREE.BufferAttribute(colors, 3));

  const mat = new THREE.MeshBasicMaterial({ vertexColors: true });
  const mesh = new THREE.Mesh(geo, mat);
  group.add(mesh);

  return {
    group,
    update: (t) => {
      mesh.rotation.x = t * 0.3;
      mesh.rotation.y = t * 0.45;
    },
    distance: 9,
    height: 0,
    target: new THREE.Vector3(0, 0, 0),
  };
}

function buildOcean() {
  const group = new THREE.Group();

  const W = 40;
  const SEG = 70;
  const geo = new THREE.PlaneGeometry(W, W, SEG, SEG);
  const positions = geo.attributes.position;
  const colors = new Float32Array(positions.count * 3);
  geo.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  geo.rotateX(-Math.PI / 2);

  const mat = new THREE.MeshBasicMaterial({ vertexColors: true });
  const mesh = new THREE.Mesh(geo, mat);
  group.add(mesh);

  // Sun
  const sun = new THREE.Mesh(
    new THREE.SphereGeometry(1.4, 32, 32),
    new THREE.MeshBasicMaterial({ color: 0xff2030 })
  );
  sun.position.set(0, 3.2, -10);
  group.add(sun);

  // Reflection trail (a stretched red plane)
  const trailGeo = new THREE.PlaneGeometry(1.2, 9, 1, 24);
  const trailPos = trailGeo.attributes.position;
  const trailColors = new Float32Array(trailPos.count * 3);
  for (let i = 0; i < trailPos.count; i++) {
    const v = (trailPos.getY(i) + 4.5) / 9;
    trailColors[i * 3 + 0] = 1.0 * (1 - v) * 0.85;
    trailColors[i * 3 + 1] = 0;
    trailColors[i * 3 + 2] = 0;
  }
  trailGeo.setAttribute('color', new THREE.BufferAttribute(trailColors, 3));
  trailGeo.rotateX(-Math.PI / 2);
  const trail = new THREE.Mesh(trailGeo, new THREE.MeshBasicMaterial({ vertexColors: true, transparent: true, opacity: 0.8 }));
  trail.position.set(0, 0.02, -4.5);
  group.add(trail);

  return {
    group,
    update: (t) => {
      const pos = positions;
      for (let i = 0; i < pos.count; i++) {
        const x = pos.getX(i);
        const z = pos.getZ(i);
        const h =
          Math.sin(x * 0.55 + t * 1.2) * 0.35 +
          Math.cos(z * 0.4 + t * 0.9) * 0.4 +
          Math.sin((x + z) * 0.3 + t * 0.7) * 0.25;
        pos.setY(i, h);

        // Higher = brighter blue. Crest near zero z = redder (reflection).
        const distSun = Math.abs(x) + Math.max(0, -z) * 0.2;
        const heat = Math.max(0, 1 - distSun * 0.5) * Math.max(0, h + 0.3);
        const blue = Math.max(0, h + 0.6) * 0.9;
        colors[i * 3 + 0] = heat * 1.1;
        colors[i * 3 + 1] = 0;
        colors[i * 3 + 2] = blue;
      }
      pos.needsUpdate = true;
      geo.attributes.color.needsUpdate = true;
    },
    distance: 10,
    height: 2.4,
    target: new THREE.Vector3(0, 0.5, 0),
  };
}

function buildPlanet() {
  const group = new THREE.Group();

  // Planet
  const geo = new THREE.SphereGeometry(2.6, 80, 60);
  const positions = geo.attributes.position;
  const colors = new Float32Array(positions.count * 3);
  for (let i = 0; i < positions.count; i++) {
    const x = positions.getX(i);
    const y = positions.getY(i);
    const z = positions.getZ(i);
    // Displace by noise to give a planet surface
    const n = noise2d(x * 1.0, y * 1.0) * 0.18 + noise2d(z * 1.2, y * 1.5) * 0.1;
    const len = Math.sqrt(x * x + y * y + z * z);
    const s = 1 + n * 0.5;
    positions.setX(i, (x / len) * 2.6 * s);
    positions.setY(i, (y / len) * 2.6 * s);
    positions.setZ(i, (z / len) * 2.6 * s);

    // North pole red, south pole blue, equator dimmer
    const lat = y / 2.6;
    const heat = Math.max(0, lat);
    const cold = Math.max(0, -lat);
    const speckle = (Math.sin(x * 5) * Math.cos(z * 5) * 0.5 + 0.5);
    colors[i * 3 + 0] = heat * (0.6 + speckle * 0.5) + Math.max(0, n) * 0.5;
    colors[i * 3 + 1] = 0;
    colors[i * 3 + 2] = cold * (0.6 + speckle * 0.5) + Math.max(0, -n) * 0.3;
  }
  geo.setAttribute('color', new THREE.BufferAttribute(colors, 3));
  geo.computeVertexNormals();

  const planet = new THREE.Mesh(
    geo,
    new THREE.MeshBasicMaterial({ vertexColors: true })
  );
  group.add(planet);

  // Moon
  const moon = new THREE.Mesh(
    new THREE.SphereGeometry(0.42, 24, 16),
    new THREE.MeshBasicMaterial({ color: 0xffffff })
  );
  group.add(moon);

  // Ring (thin torus, white)
  const ring = new THREE.Mesh(
    new THREE.TorusGeometry(3.7, 0.04, 8, 100),
    new THREE.MeshBasicMaterial({ color: 0x6688ff })
  );
  ring.rotation.x = Math.PI / 2.4;
  group.add(ring);

  return {
    group,
    update: (t) => {
      planet.rotation.y = t * 0.25;
      moon.position.set(
        Math.cos(t * 0.6) * 5.2,
        Math.sin(t * 0.4) * 0.6,
        Math.sin(t * 0.6) * 5.2
      );
      ring.rotation.z = t * 0.12;
    },
    distance: 9,
    height: 0.5,
    target: new THREE.Vector3(0, 0, 0),
  };
}

function buildTunnel() {
  const group = new THREE.Group();

  const RINGS = 24;
  const rings = [];
  for (let i = 0; i < RINGS; i++) {
    const isRed = i % 4 === 0;
    const isBlue = i % 4 === 2;
    const color = isRed ? 0xff2030 : isBlue ? 0x3050ff : 0x202040;
    const ring = new THREE.Mesh(
      new THREE.TorusGeometry(2.5, 0.08, 6, 32),
      new THREE.MeshBasicMaterial({ color })
    );
    ring.position.z = -i * 2.5;
    group.add(ring);
    rings.push(ring);
  }

  // Long bars along the tunnel
  for (let i = 0; i < 6; i++) {
    const ang = (i / 6) * Math.PI * 2;
    const bar = new THREE.Mesh(
      new THREE.BoxGeometry(0.05, 0.05, RINGS * 2.5),
      new THREE.MeshBasicMaterial({ color: 0x000050 })
    );
    bar.position.set(Math.cos(ang) * 2.5, Math.sin(ang) * 2.5, -RINGS * 1.25);
    group.add(bar);
  }

  return {
    group,
    update: (t) => {
      const N = rings.length;
      const spacing = 2.5;
      const totalLen = N * spacing;
      const speed = 4;
      for (let i = 0; i < N; i++) {
        let z = 5 - i * spacing - t * speed;
        // wrap into [5 - totalLen, 5]
        z = ((z - 5) % totalLen + totalLen) % totalLen + (5 - totalLen);
        rings[i].position.z = z;
      }
    },
    distance: 0.1,
    height: 0,
    target: new THREE.Vector3(0, 0, -10),
    fixedCamera: (camera, angle) => {
      camera.position.set(Math.cos(angle) * 0.4, Math.sin(angle) * 0.4, 5);
      camera.lookAt(0, 0, -5);
    },
  };
}

const BUILDERS = {
  terrain: buildTerrain,
  torus: buildTorus,
  ocean: buildOcean,
  planet: buildPlanet,
  tunnel: buildTunnel,
};

function disposeGroup(group) {
  group.traverse((obj) => {
    if (obj.geometry) obj.geometry.dispose();
    if (obj.material) {
      if (Array.isArray(obj.material)) obj.material.forEach((m) => m.dispose());
      else obj.material.dispose();
    }
  });
}

/* ----------------------------------------------------------
   The Component
   ---------------------------------------------------------- */

export default function M5DialSimulator() {
  const canvasRef = useRef(null);
  const bezelRef = useRef(null);
  const knobMarkerRef = useRef(null);
  const refs = useRef({});

  const [sceneId, setSceneId] = useState('terrain');
  const [pixelSize, setPixelSize] = useState(2);
  const [autoRotate, setAutoRotate] = useState(0.25);
  const [paletteKey, setPaletteKey] = useState('classic');
  const [contrast, setContrast] = useState(1.4);
  const [maskRadius, setMaskRadius] = useState(0.97);
  const [clockTime, setClockTime] = useState('10:42');

  // Live clock
  useEffect(() => {
    const tick = () => {
      const d = new Date();
      const hh = String(d.getHours()).padStart(2, '0');
      const mm = String(d.getMinutes()).padStart(2, '0');
      setClockTime(`${hh}:${mm}`);
    };
    tick();
    const i = setInterval(tick, 10_000);
    return () => clearInterval(i);
  }, []);

  // Init Three.js once
  useEffect(() => {
    const canvas = canvasRef.current;
    const renderer = new THREE.WebGLRenderer({ canvas, antialias: false, alpha: false });
    renderer.setSize(480, 480, false);
    renderer.setPixelRatio(1);
    renderer.setClearColor(0x000000, 1);

    const lowRes = new THREE.WebGLRenderTarget(240, 240, {
      minFilter: THREE.NearestFilter,
      magFilter: THREE.NearestFilter,
      format: THREE.RGBAFormat,
    });

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x000000);
    const camera = new THREE.PerspectiveCamera(50, 1, 0.05, 200);

    // Dither pipeline
    const ditherScene = new THREE.Scene();
    const ditherCamera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
    const ditherMat = new THREE.ShaderMaterial({
      uniforms: {
        tDiffuse: { value: lowRes.texture },
        uResolution: { value: new THREE.Vector2(240, 240) },
        uPixelSize: { value: 2.0 },
        uColorWarm: { value: new THREE.Color(PALETTES.classic.warm) },
        uColorCool: { value: new THREE.Color(PALETTES.classic.cool) },
        uColorHigh: { value: new THREE.Color(PALETTES.classic.high) },
        uMaskRadius: { value: 0.97 },
        uContrast: { value: 1.4 },
      },
      vertexShader: DITHER_VS,
      fragmentShader: DITHER_FS,
    });
    const quad = new THREE.Mesh(new THREE.PlaneGeometry(2, 2), ditherMat);
    ditherScene.add(quad);

    refs.current = {
      renderer,
      scene,
      camera,
      lowRes,
      ditherScene,
      ditherCamera,
      ditherMat,
      cameraAngle: 0,
      autoRotate: 0.25,
      activeScene: null,
    };

    let frameId;
    let lastT = performance.now();

    const animate = () => {
      const now = performance.now();
      const dt = Math.min(0.1, (now - lastT) / 1000);
      lastT = now;
      const r = refs.current;

      r.cameraAngle += r.autoRotate * dt;

      // Update knob visual smoothly (DOM, no React rerender)
      if (bezelRef.current) {
        bezelRef.current.style.transform = `rotate(${r.cameraAngle * 60}deg)`;
      }
      if (knobMarkerRef.current) {
        knobMarkerRef.current.style.transform = `rotate(${r.cameraAngle * 60}deg)`;
      }

      if (r.activeScene?.update) r.activeScene.update(now / 1000, dt);

      if (r.activeScene) {
        if (r.activeScene.fixedCamera) {
          r.activeScene.fixedCamera(camera, r.cameraAngle);
        } else {
          const dist = r.activeScene.distance;
          const h = r.activeScene.height;
          const tgt = r.activeScene.target;
          camera.position.set(
            tgt.x + Math.sin(r.cameraAngle) * dist,
            tgt.y + h,
            tgt.z + Math.cos(r.cameraAngle) * dist
          );
          camera.lookAt(tgt);
        }
      }

      renderer.setRenderTarget(lowRes);
      renderer.render(scene, camera);
      renderer.setRenderTarget(null);
      renderer.render(ditherScene, ditherCamera);

      frameId = requestAnimationFrame(animate);
    };
    animate();

    return () => {
      cancelAnimationFrame(frameId);
      if (refs.current.activeScene) {
        scene.remove(refs.current.activeScene.group);
        disposeGroup(refs.current.activeScene.group);
      }
      lowRes.dispose();
      ditherMat.dispose();
      renderer.dispose();
    };
  }, []);

  // Build / swap scene on sceneId change
  useEffect(() => {
    const r = refs.current;
    if (!r.scene) return;

    if (r.activeScene?.group) {
      r.scene.remove(r.activeScene.group);
      disposeGroup(r.activeScene.group);
    }

    const built = BUILDERS[sceneId]();
    r.scene.add(built.group);
    r.activeScene = built;
  }, [sceneId]);

  // Push parameter changes into refs / uniforms
  useEffect(() => {
    const r = refs.current;
    if (!r.ditherMat) return;
    r.ditherMat.uniforms.uPixelSize.value = pixelSize;
    r.ditherMat.uniforms.uContrast.value = contrast;
    r.ditherMat.uniforms.uMaskRadius.value = maskRadius;
    const pal = PALETTES[paletteKey];
    r.ditherMat.uniforms.uColorWarm.value.set(pal.warm);
    r.ditherMat.uniforms.uColorCool.value.set(pal.cool);
    r.ditherMat.uniforms.uColorHigh.value.set(pal.high);
  }, [pixelSize, contrast, maskRadius, paletteKey]);

  useEffect(() => {
    refs.current.autoRotate = autoRotate;
  }, [autoRotate]);

  // Pointer / wheel handlers on the dial
  const dragRef = useRef({ active: false, x: 0 });
  const dialBoxRef = useRef(null);

  // Wheel needs non-passive listener so preventDefault stops page scroll
  useEffect(() => {
    const el = dialBoxRef.current;
    if (!el) return;
    const handler = (e) => {
      e.preventDefault();
      refs.current.cameraAngle += e.deltaY * 0.0035;
    };
    el.addEventListener('wheel', handler, { passive: false });
    return () => el.removeEventListener('wheel', handler);
  }, []);

  const onPointerDown = (e) => {
    dragRef.current.active = true;
    dragRef.current.x = e.clientX;
    e.currentTarget.setPointerCapture?.(e.pointerId);
  };
  const onPointerMove = (e) => {
    if (!dragRef.current.active) return;
    const dx = e.clientX - dragRef.current.x;
    dragRef.current.x = e.clientX;
    refs.current.cameraAngle += dx * 0.012;
  };
  const onPointerUp = (e) => {
    dragRef.current.active = false;
    e.currentTarget.releasePointerCapture?.(e.pointerId);
  };

  // Keyboard: arrows to change scene
  useEffect(() => {
    const onKey = (e) => {
      if (e.key === 'ArrowRight' || e.key === 'ArrowDown') {
        e.preventDefault();
        const idx = SCENES.findIndex((s) => s.id === sceneId);
        setSceneId(SCENES[(idx + 1) % SCENES.length].id);
      } else if (e.key === 'ArrowLeft' || e.key === 'ArrowUp') {
        e.preventDefault();
        const idx = SCENES.findIndex((s) => s.id === sceneId);
        setSceneId(SCENES[(idx - 1 + SCENES.length) % SCENES.length].id);
      } else if (e.key === ' ') {
        e.preventDefault();
        // tap = next scene
        const idx = SCENES.findIndex((s) => s.id === sceneId);
        setSceneId(SCENES[(idx + 1) % SCENES.length].id);
      }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [sceneId]);

  const currentScene = SCENES.find((s) => s.id === sceneId);
  const palette = PALETTES[paletteKey];

  const cycle = () => {
    const idx = SCENES.findIndex((s) => s.id === sceneId);
    setSceneId(SCENES[(idx + 1) % SCENES.length].id);
  };

  return (
    <div className="min-h-screen w-full bg-black text-white relative overflow-hidden" style={{ fontFamily: "'JetBrains Mono', ui-monospace, monospace" }}>
      <style>{`
        @import url('https://fonts.googleapis.com/css2?family=VT323&family=JetBrains+Mono:wght@400;600;700&display=swap');
        body { background:#000; }
        .pix { font-family: 'VT323', monospace; letter-spacing: 0.04em; }
        .grid-bg {
          background-image:
            linear-gradient(rgba(80,80,160,0.06) 1px, transparent 1px),
            linear-gradient(90deg, rgba(80,80,160,0.06) 1px, transparent 1px);
          background-size: 32px 32px;
        }
        input[type=range].knob {
          -webkit-appearance:none; appearance:none; height: 4px;
          background: linear-gradient(to right, #3050ff 0%, #3050ff var(--p,50%), #1a1a22 var(--p,50%), #1a1a22 100%);
          border:none; outline:none; border-radius:2px; width:100%;
        }
        input[type=range].knob::-webkit-slider-thumb {
          -webkit-appearance:none; appearance:none;
          width:14px; height:14px; border-radius:0; background:#ff2940;
          border:2px solid #fff; cursor:pointer;
        }
        input[type=range].knob::-moz-range-thumb {
          width:14px; height:14px; border-radius:0; background:#ff2940;
          border:2px solid #fff; cursor:pointer;
        }
        .canvas-px { image-rendering: pixelated; image-rendering: crisp-edges; }
        .bezel-notch { transform-origin: 280px 280px; }
        .glow-red { filter: drop-shadow(0 0 8px rgba(255,41,64,0.45)); }
        .glow-blue { filter: drop-shadow(0 0 6px rgba(48,80,255,0.5)); }
        @keyframes blink { 0%,49%{opacity:1} 50%,100%{opacity:0.15} }
        .blink { animation: blink 1.6s steps(2) infinite; }
        @keyframes scan { from{transform:translateY(-100%)} to{transform:translateY(100%)} }
        .scan { animation: scan 7s linear infinite; }
      `}</style>

      {/* faint background grid + horizontal scanline */}
      <div className="absolute inset-0 grid-bg opacity-50 pointer-events-none" />
      <div className="absolute inset-0 pointer-events-none overflow-hidden">
        <div className="scan absolute left-0 right-0 h-px bg-gradient-to-b from-transparent via-blue-500/20 to-transparent" />
      </div>

      {/* Top bar */}
      <div className="relative flex items-center justify-between px-6 py-3 border-b border-white/10 z-10">
        <div className="flex items-center gap-3">
          <div className="w-2 h-2 bg-red-500 blink" />
          <span className="pix text-2xl tracking-widest text-white">M5 DIAL // SIM</span>
          <span className="pix text-lg text-blue-400">v1.0.0</span>
        </div>
        <div className="flex items-center gap-6 pix text-lg text-white/60">
          <span>{clockTime}</span>
          <span>STATUS: <span className="text-red-500">ACTIVE</span></span>
          <span className="text-blue-400">PWR 100%</span>
        </div>
      </div>

      <div className="relative z-10 flex flex-col lg:flex-row gap-8 p-8 max-w-[1400px] mx-auto">
        {/* ---------- DIAL ---------- */}
        <div className="flex-1 flex flex-col items-center justify-center">
          <div className="relative" style={{ width: 560, height: 560 }}>
            {/* Outer corner brackets */}
            <CornerBrackets />

            {/* Rotating bezel ring with notches */}
            <svg
              ref={bezelRef}
              className="absolute inset-0"
              viewBox="0 0 560 560"
              style={{ transform: 'rotate(0deg)' }}
            >
              <defs>
                <radialGradient id="bezelGrad" cx="50%" cy="40%" r="60%">
                  <stop offset="0%" stopColor="#222228" />
                  <stop offset="70%" stopColor="#0a0a0d" />
                  <stop offset="100%" stopColor="#000" />
                </radialGradient>
              </defs>
              <circle cx="280" cy="280" r="278" fill="url(#bezelGrad)" stroke="#222" strokeWidth="2" />
              <circle cx="280" cy="280" r="244" fill="none" stroke="#1a1a22" strokeWidth="1" />
              {/* Notches */}
              {Array.from({ length: 60 }).map((_, i) => {
                const isMajor = i % 5 === 0;
                const len = isMajor ? 14 : 6;
                return (
                  <line
                    key={i}
                    x1="280"
                    y1={280 - 250}
                    x2="280"
                    y2={280 - 250 + len}
                    stroke={isMajor ? '#5a78ff' : '#2a2a35'}
                    strokeWidth={isMajor ? 1.6 : 1}
                    transform={`rotate(${i * 6} 280 280)`}
                  />
                );
              })}
              {/* Top indicator */}
              <polygon points="280,18 274,32 286,32" fill="#ff2940" />
            </svg>

            {/* Display canvas inside circular cutout */}
            <div
              ref={dialBoxRef}
              className="absolute"
              style={{
                left: 40,
                top: 40,
                width: 480,
                height: 480,
                borderRadius: '50%',
                overflow: 'hidden',
                background: '#000',
                boxShadow:
                  'inset 0 0 0 2px #1a1a22, inset 0 0 30px rgba(0,0,0,0.9), 0 0 40px rgba(48,80,255,0.08)',
                touchAction: 'none',
                cursor: 'grab',
              }}
              onPointerDown={onPointerDown}
              onPointerMove={onPointerMove}
              onPointerUp={onPointerUp}
              onClick={(e) => {
                // tap near center = next scene
                const r = e.currentTarget.getBoundingClientRect();
                const cx = r.left + r.width / 2;
                const cy = r.top + r.height / 2;
                const dx = e.clientX - cx;
                const dy = e.clientY - cy;
                if (Math.sqrt(dx * dx + dy * dy) < 60) cycle();
              }}
            >
              <canvas
                ref={canvasRef}
                width={480}
                height={480}
                className="canvas-px block"
                style={{ width: 480, height: 480, imageRendering: 'pixelated' }}
              />

              {/* Overlay UI on the screen */}
              <div className="absolute inset-0 pointer-events-none flex flex-col items-center justify-between p-8">
                <div className="pix text-red-500 text-3xl tracking-[0.3em] glow-red mt-4">
                  {currentScene.name}
                </div>
                <div className="flex flex-col items-center gap-2">
                  <div className="flex gap-2 mb-1">
                    {SCENES.map((s) => (
                      <div
                        key={s.id}
                        className="w-2 h-2 rounded-full"
                        style={{
                          background: s.id === sceneId ? '#ff2940' : '#444',
                          boxShadow: s.id === sceneId ? '0 0 8px #ff2940' : 'none',
                        }}
                      />
                    ))}
                  </div>
                  <div className="pix text-blue-400 text-xl tracking-widest opacity-80">
                    {currentScene.subtitle}
                  </div>
                </div>
              </div>
            </div>

            {/* Bottom knob marker (independent indicator dot) */}
            <div
              ref={knobMarkerRef}
              className="absolute inset-0 pointer-events-none"
              style={{ transformOrigin: 'center' }}
            >
              <div
                className="absolute"
                style={{
                  left: '50%',
                  top: 8,
                  width: 4,
                  height: 16,
                  background: '#ff2940',
                  transform: 'translateX(-50%)',
                  boxShadow: '0 0 8px #ff2940',
                }}
              />
            </div>
          </div>

          {/* Tactile hints */}
          <div className="mt-6 flex gap-8 pix text-blue-300/70 text-lg">
            <span>SCROLL ◌ ROTATE</span>
            <span>DRAG ◌ ORBIT</span>
            <span>TAP ◌ NEXT</span>
            <span>↑↓←→ ◌ SCENE</span>
          </div>
        </div>

        {/* ---------- SIDEBAR ---------- */}
        <div className="w-full lg:w-[360px] space-y-5 shrink-0">
          <Panel title="CONFIG // SCENE">
            <div className="grid grid-cols-2 gap-2">
              {SCENES.map((s) => {
                const active = s.id === sceneId;
                return (
                  <button
                    key={s.id}
                    onClick={() => setSceneId(s.id)}
                    className="pix text-left px-3 py-3 border transition relative overflow-hidden"
                    style={{
                      background: active ? 'rgba(255,41,64,0.12)' : 'rgba(48,80,255,0.04)',
                      borderColor: active ? '#ff2940' : '#1a1a28',
                      color: active ? '#ff2940' : '#9aa6d8',
                    }}
                  >
                    <span className="text-2xl mr-2 align-middle">{s.glyph}</span>
                    <span className="text-xl align-middle tracking-wider">{s.name}</span>
                    <div className="text-xs opacity-50 mt-1">{s.subtitle}</div>
                    {active && (
                      <div className="absolute right-2 top-2 w-1.5 h-1.5 bg-red-500 blink" />
                    )}
                  </button>
                );
              })}
            </div>
          </Panel>

          <Panel title="CONFIG // VISUAL">
            <Slider
              label="ROTATION"
              value={autoRotate}
              onChange={setAutoRotate}
              min={-1.5}
              max={1.5}
              step={0.05}
              format={(v) => v.toFixed(2)}
            />
            <Slider
              label="PIXEL SIZE"
              value={pixelSize}
              onChange={setPixelSize}
              min={1}
              max={6}
              step={1}
              format={(v) => `${v}px`}
            />
            <Slider
              label="CONTRAST"
              value={contrast}
              onChange={setContrast}
              min={0.6}
              max={2.5}
              step={0.05}
              format={(v) => v.toFixed(2)}
            />
            <Slider
              label="APERTURE"
              value={maskRadius}
              onChange={setMaskRadius}
              min={0.4}
              max={1.0}
              step={0.01}
              format={(v) => `${Math.round(v * 100)}%`}
            />
          </Panel>

          <Panel title="CONFIG // PALETTE">
            <div className="space-y-1">
              {Object.entries(PALETTES).map(([key, p]) => {
                const active = key === paletteKey;
                return (
                  <button
                    key={key}
                    onClick={() => setPaletteKey(key)}
                    className="w-full flex items-center gap-3 px-3 py-2 pix transition"
                    style={{
                      background: active ? 'rgba(255,41,64,0.10)' : 'transparent',
                      borderLeft: active ? '3px solid #ff2940' : '3px solid transparent',
                      color: active ? '#fff' : '#778',
                    }}
                  >
                    <div className="flex gap-0 border border-white/10">
                      <div style={{ background: '#000', width: 16, height: 16 }} />
                      <div style={{ background: p.warm, width: 16, height: 16 }} />
                      <div style={{ background: p.cool, width: 16, height: 16 }} />
                      <div style={{ background: p.high, width: 16, height: 16 }} />
                    </div>
                    <span className="text-lg tracking-widest">{p.label}</span>
                    {active && (
                      <span className="ml-auto text-red-500 text-xs">● LIVE</span>
                    )}
                  </button>
                );
              })}
            </div>
          </Panel>

          <Panel title="CONFIG // OUTPUT">
            <div className="pix text-blue-300/80 text-sm leading-relaxed space-y-1">
              <Row k="RENDER" v="240×240 → 480×480" />
              <Row k="DITHER" v="BAYER 4×4 ORDERED" />
              <Row k="COLORS" v="4 (BLK · WRM · COOL · HI)" />
              <Row k="ENCODER" v={`${(refs.current.cameraAngle ?? 0).toFixed(2)} rad`} />
              <Row k="SCENE" v={currentScene.name} />
            </div>
          </Panel>
        </div>
      </div>

      <div className="pix text-blue-400/40 text-xs text-center pb-4 relative z-10 tracking-widest">
        ▌ DEVICE OK ▌ SIGNAL LOCK ▌ M5STACK FORM FACTOR EMULATION ▌
      </div>
    </div>
  );
}

/* ----------------------------------------------------------
   Sub-components
   ---------------------------------------------------------- */

function Panel({ title, children }) {
  return (
    <div className="border border-white/10 bg-black/60 backdrop-blur-sm">
      <div className="flex items-center justify-between px-3 py-1.5 border-b border-white/10 bg-white/[0.02]">
        <span className="pix text-blue-400 text-base tracking-widest">{title}</span>
        <span className="text-red-500 text-xs">●</span>
      </div>
      <div className="p-3">{children}</div>
    </div>
  );
}

function Slider({ label, value, onChange, min, max, step, format }) {
  const pct = ((value - min) / (max - min)) * 100;
  return (
    <div className="mb-3 last:mb-0">
      <div className="flex justify-between pix text-sm mb-1">
        <span className="text-blue-300/80 tracking-widest">{label}</span>
        <span className="text-red-400">{format(value)}</span>
      </div>
      <input
        type="range"
        className="knob"
        style={{ ['--p']: `${pct}%` }}
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(parseFloat(e.target.value))}
      />
    </div>
  );
}

function Row({ k, v }) {
  return (
    <div className="flex justify-between">
      <span className="text-blue-400/70 tracking-widest">{k}</span>
      <span className="text-white/80">{v}</span>
    </div>
  );
}

function CornerBrackets() {
  const Bracket = ({ style }) => (
    <div className="absolute w-6 h-6 border-blue-400/60" style={style} />
  );
  return (
    <>
      <Bracket style={{ left: -8, top: -8, borderLeft: '2px solid', borderTop: '2px solid' }} />
      <Bracket style={{ right: -8, top: -8, borderRight: '2px solid', borderTop: '2px solid' }} />
      <Bracket style={{ left: -8, bottom: -8, borderLeft: '2px solid', borderBottom: '2px solid' }} />
      <Bracket style={{ right: -8, bottom: -8, borderRight: '2px solid', borderBottom: '2px solid' }} />
    </>
  );
}

import * as THREE from "three";
import { STLLoader } from "three/examples/jsm/loaders/STLLoader.js";

type LinkVisual = {
  color: THREE.Color;
  meshPath: string;
};

type JointTransform = {
  parent: string;
  position: THREE.Vector3;
  rotation: THREE.Euler;
};

const stlLoader = new STLLoader();
let robotModelPromise: Promise<THREE.Group> | null = null;

function parseVector(value: string | null | undefined) {
  const parts = (value ?? "0 0 0")
    .trim()
    .split(/\s+/)
    .map((item) => Number(item));
  return [parts[0] ?? 0, parts[1] ?? 0, parts[2] ?? 0] as const;
}

function parseColor(value: string | null | undefined) {
  const parts = (value ?? "0.85 0.85 0.85 1")
    .trim()
    .split(/\s+/)
    .map((item) => Number(item));
  return new THREE.Color(parts[0] ?? 0.85, parts[1] ?? 0.85, parts[2] ?? 0.85);
}

function packageMeshToPublicPath(filename: string) {
  return filename.replace("package://rai_robot_urdf/", "/robot-model/");
}

async function buildRobotModel() {
  const response = await fetch("/robot-model/mini_mec_robot.urdf");
  if (!response.ok) {
    throw new Error("Cannot load robot URDF");
  }

  const urdfText = await response.text();
  const xml = new DOMParser().parseFromString(urdfText, "application/xml");
  const links = new Map<string, LinkVisual>();
  const joints = new Map<string, JointTransform>();

  for (const linkNode of Array.from(xml.querySelectorAll("robot > link"))) {
    const linkName = linkNode.getAttribute("name");
    const meshNode = linkNode.querySelector("visual geometry mesh");
    if (!linkName || !meshNode) continue;

    links.set(linkName, {
      color: parseColor(linkNode.querySelector("visual material color")?.getAttribute("rgba")),
      meshPath: packageMeshToPublicPath(meshNode.getAttribute("filename") ?? ""),
    });
  }

  for (const jointNode of Array.from(xml.querySelectorAll("robot > joint"))) {
    const child = jointNode.querySelector("child")?.getAttribute("link");
    const parent = jointNode.querySelector("parent")?.getAttribute("link");
    if (!child || !parent) continue;

    const originNode = jointNode.querySelector("origin");
    const [x, y, z] = parseVector(originNode?.getAttribute("xyz"));
    const [rr, rp, ry] = parseVector(originNode?.getAttribute("rpy"));
    joints.set(child, {
      parent,
      position: new THREE.Vector3(x, y, z),
      rotation: new THREE.Euler(rr, rp, ry, "XYZ"),
    });
  }

  const worldTransforms = new Map<string, THREE.Matrix4>();
  worldTransforms.set("base_link", new THREE.Matrix4().identity());

  const resolveTransform = (linkName: string): THREE.Matrix4 => {
    const cached = worldTransforms.get(linkName);
    if (cached) return cached;

    const joint = joints.get(linkName);
    if (!joint) {
      const identity = new THREE.Matrix4().identity();
      worldTransforms.set(linkName, identity);
      return identity;
    }

    const parentMatrix = resolveTransform(joint.parent);
    const localMatrix = new THREE.Matrix4().compose(
      joint.position,
      new THREE.Quaternion().setFromEuler(joint.rotation),
      new THREE.Vector3(1, 1, 1),
    );
    const worldMatrix = parentMatrix.clone().multiply(localMatrix);
    worldTransforms.set(linkName, worldMatrix);
    return worldMatrix;
  };

  const group = new THREE.Group();
  for (const [linkName, visual] of links.entries()) {
    const geometry = await stlLoader.loadAsync(visual.meshPath);
    geometry.computeVertexNormals();
    const material = new THREE.MeshStandardMaterial({
      color: visual.color,
      metalness: 0.18,
      roughness: 0.72,
    });
    const mesh = new THREE.Mesh(geometry, material);
    mesh.applyMatrix4(resolveTransform(linkName));
    mesh.castShadow = true;
    mesh.receiveShadow = true;
    group.add(mesh);
  }

  const bounds = new THREE.Box3().setFromObject(group);
  const center = bounds.getCenter(new THREE.Vector3());
  group.position.sub(center);
  return group;
}

export async function loadRobotModel() {
  if (!robotModelPromise) {
    robotModelPromise = buildRobotModel();
  }
  const model = await robotModelPromise;
  return model.clone(true);
}

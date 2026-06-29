import type { NextConfig } from "next";

const defaultAllowedDevOrigins = ["100.116.199.115"];
const configuredAllowedDevOrigins =
  process.env.NEXT_PUBLIC_ALLOWED_DEV_ORIGINS?.split(",")
    .map((origin) => origin.trim())
    .filter((origin) => origin.length > 0) ?? [];

const nextConfig: NextConfig = {
  allowedDevOrigins: Array.from(new Set([...defaultAllowedDevOrigins, ...configuredAllowedDevOrigins])),
};

export default nextConfig;

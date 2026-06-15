#!/bin/bash
# ──────────────────────────────────────────────────────────────
# Enable SSL for rai-k63.me
# Usage: bash scripts/enable-ssl.sh your@email.com
# ──────────────────────────────────────────────────────────────
set -euo pipefail

DOMAIN="rai-k63.me"
EMAIL="${1:-admin@${DOMAIN}}"
COMPOSE="docker-compose.prod.yml"

echo ""
echo "=== Step 1: Request SSL certificate ==="
docker compose -f "${COMPOSE}" run --rm certbot certonly \
    --webroot \
    --webroot-path=/var/www/certbot \
    --email "${EMAIL}" \
    --agree-tos \
    --no-eff-email \
    -d "${DOMAIN}" \
    -d "www.${DOMAIN}"

echo ""
echo "=== Step 2: Switch nginx to HTTPS mode ==="
cp nginx/conf.d/default-ssl.conf.template nginx/conf.d/default.conf

echo ""
echo "=== Step 3: Reload nginx ==="
docker compose -f "${COMPOSE}" exec nginx nginx -s reload

echo ""
echo "=== Done! ==="
echo "Site is now live at: https://${DOMAIN}"
echo ""
echo "Remember to update .env:"
echo "  NEXT_PUBLIC_API_URL=https://${DOMAIN}"
echo "  NEXT_PUBLIC_WS_URL=wss://${DOMAIN}"
echo "Then: docker compose -f ${COMPOSE} up -d --force-recreate client"

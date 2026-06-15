FROM node:22-alpine

WORKDIR /app

# Configure npm to be resilient against slow/unstable networks
RUN npm config set fetch-retry-mintimeout 20000 \
 && npm config set fetch-retry-maxtimeout 120000 \
 && npm config set fetch-retries 5 \
 && npm config set fetch-timeout 300000 \
 && npm config set prefer-offline true

# Install dependencies (cache mount avoids re-download on rebuild)
COPY package.json package-lock.json* ./
RUN --mount=type=cache,target=/root/.npm \
    npm ci --prefer-offline || npm ci

# Copy source code
COPY . .

ARG NEXT_PUBLIC_API_URL
ARG NEXT_PUBLIC_WS_URL
ENV NEXT_PUBLIC_API_URL=$NEXT_PUBLIC_API_URL
ENV NEXT_PUBLIC_WS_URL=$NEXT_PUBLIC_WS_URL

# Build once and run without file watching inside Docker.
RUN npm run build -- --webpack

CMD ["npm", "run", "start"]

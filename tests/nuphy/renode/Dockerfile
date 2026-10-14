# Renode for the NuPhy tests, native on the host's architecture (arm64 Macs, x86-64 CI).
# docker build -t nuphy-renode tests/nuphy/renode
FROM debian:bookworm-slim
ARG RENODE_VERSION=1.17.0
ARG TARGETARCH
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates curl libicu72 \
 && case "$TARGETARCH" in arm64) pkg=linux-arm64-portable ;; *) pkg=linux-portable ;; esac \
 && curl -fsSL "https://github.com/renode/renode/releases/download/v$RENODE_VERSION/renode-$RENODE_VERSION.$pkg.tar.gz" | tar xz -C /opt \
 && mv /opt/renode_*-portable /opt/renode \
 && apt-get purge -y curl && apt-get autoremove -y && rm -rf /var/lib/apt/lists/*
ENV PATH=/opt/renode:$PATH
WORKDIR /work

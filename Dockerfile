FROM ghcr.io/wiiu-env/devkitppc:20260504

COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260503 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libmocha:20260331 /artifacts $DEVKITPRO

ENV PATH "$PATH:/opt/devkitpro/tools/bin"

WORKDIR /project

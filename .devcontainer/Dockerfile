FROM ubuntu:22.04
ARG USERNAME=sonata
# Install nix, git and cmake (required to build sonata-system software,
# temporary addition to this dockerfile until we have software build setup in
# sonata-software).
RUN apt update -y
RUN apt install curl git sudo -y

# Setup user
RUN adduser --disabled-password --gecos "" --uid 1001 $USERNAME \
    && usermod -aG sudo $USERNAME \
    && sed -i 's/\(%sudo.*\) ALL/\1 NOPASSWD:ALL/' /etc/sudoers

# Switch to user
USER $USERNAME

# Install nix
RUN curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install linux \
  --extra-conf "sandbox = false" \
  --init none \
  --no-confirm

RUN sudo chown -R $USERNAME /nix

ENV PATH="${PATH}:/nix/var/nix/profiles/default/bin"

# Setup lowRISC nix cache and devshell bash prompt
RUN sudo bash -c 'echo "substituters = https://cache.nixos.org https://nix-cache.lowrisc.org/public/" >> /etc/nix/nix.conf' \
  && sudo bash -c 'echo "trusted-public-keys = cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY=  nix-cache.lowrisc.org-public-1:O6JLD0yXzaJDPiQW1meVu32JIDViuaPtGDfjlOopU7o=" >> /etc/nix/nix.conf' \
  && sudo bash -c "echo trusted-users = $USERNAME >> /etc/nix/nix.conf" \
  && sudo bash -c "echo 'bash-prompt = \[\e[1m\]Sonata \[\e[0m\]\w>\040' >> /etc/nix/nix.conf" \
  && sudo bash -c "echo 'bash-prompt-prefix =' >> /etc/nix/nix.conf" \
  && mkdir -p /home/$USERNAME/.local/share/nix \
  && echo '{"extra-substituters":{"https://nix-cache.lowrisc.org/public/":true},"extra-trusted-public-keys":{"nix-cache.lowrisc.org-public-1:O6JLD0yXzaJDPiQW1meVu32JIDViuaPtGDfjlOopU7o=":true}}' > /home/$USERNAME/.local/share/nix/trusted-settings.json

# Download/build/install everything required for the nix devshell by starting it
# to run 'true' and immediately exit
RUN nix develop "github:lowRISC/sonata-software#env-with-sim" -c "true"

ENV SHELL /bin/bash
CMD nix develop "github:lowRISC/sonata-software#env-with-sim"

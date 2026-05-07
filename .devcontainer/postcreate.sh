#!/usr/bin/env bash
set -e

sudo chown -R vscode:vscode /workspaces/LooperPlugin/build
git clone https://github.com/nathanmyles/Dotfiles ~/Dotfiles
bash ~/Dotfiles/install.sh

#!/bin/bash

# 1. Pega a última tag do Git (ex: v6.3)
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null)

# Se não existir tag ainda, começa na v6.3.0
if [ -z "$LAST_TAG" ]; then
    NEW_TAG="v6.3.0"
else
    # Lógica para aumentar o último número (ex: v6.3.0 -> v6.3.1)
    BASE=$(echo $LAST_TAG | cut -d. -f1,2)
    PATCH=$(echo $LAST_TAG | cut -d. -f3)
    if [ -z "$PATCH" ]; then PATCH=0; fi
    NEW_TAG="$BASE.$((PATCH + 1))"
fi

echo "--- Versão atual: $LAST_TAG ---"
echo "--- Lançando versão automática: $NEW_TAG ---"

# 2. Processo de Push
git add .
git commit -m "Auto-release $NEW_TAG"
git tag -a "$NEW_TAG" -m "Versão $NEW_TAG"

echo "Enviando para o GitHub (toque na YubiKey se necessário)..."
git push origin main && git push origin "$NEW_TAG"

echo "--- Pronto! $NEW_TAG está no ar ---"

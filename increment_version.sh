#!/bin/bash

# Читаем текущую версию
if [ -f "version.txt" ]; then
    VERSION=$(cat version.txt)
else
    VERSION="0.1.0"
fi

# Разбиваем версию на части
IFS='.' read -ra PARTS <<< "$VERSION"
MAJOR=${PARTS[0]}
MINOR=${PARTS[1]}  
BUILD=${PARTS[2]}

# Увеличиваем номер билда
BUILD=$((BUILD + 1))

# Формируем новую версию
NEW_VERSION="$MAJOR.$MINOR.$BUILD"

# Записываем обратно в файл
echo "$NEW_VERSION" > version.txt

echo "Version updated: $VERSION -> $NEW_VERSION"
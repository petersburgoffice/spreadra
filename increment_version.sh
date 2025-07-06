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

# Генерируем header файл с версией для использования в коде
cat > src/core/Version.h << EOF
#pragma once

#define PLUGIN_VERSION_STRING "$NEW_VERSION"
#define PLUGIN_VERSION_MAJOR $MAJOR
#define PLUGIN_VERSION_MINOR $MINOR  
#define PLUGIN_VERSION_BUILD $BUILD
EOF

echo "Generated src/core/Version.h" 
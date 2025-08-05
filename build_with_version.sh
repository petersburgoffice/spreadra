#!/bin/bash

# Скрипт сборки Spreadra с автоматическим версионированием
# Версия инкрементируется автоматически перед каждой сборкой
# Использование: ./build_with_version.sh [target]

set -e  # Остановка при ошибке

echo "🚀 Starting build with version increment..."

# ИСПРАВЛЕНО: Инкрементируем версию ПЕРЕД сборкой
echo "📈 Incrementing version..."
./increment_version.sh

# Читаем новую версию для отображения
NEW_VERSION=$(cat version.txt)
echo "🏷️  New version: v$NEW_VERSION"

# Создаем папку build если её нет
if [ ! -d "build" ]; then
    echo "📁 Creating build directory..."
    mkdir build
fi

# Переходим в папку build
cd build

# Конфигурируем CMake (здесь происходит версионирование)
echo "⚙️  Configuring CMake..."
cmake ..

# Собираем проект
if [ -z "$1" ]; then
    echo "🔨 Building all targets..."
    cmake --build .
else
    echo "🔨 Building target: $1"
    cmake --build . --target "$1"
fi

# AU плагин устанавливается автоматически через CMake
echo "📦 AU plugin installation is handled automatically by CMake"

# Возвращаемся в корневую директорию для чтения версии
cd ..

# Читаем и выводим текущую версию
CURRENT_VERSION=$(cat version.txt)
echo ""
echo "✅ Build completed successfully!"
echo "🎵 AU plugin automatically installed and ready to use"
echo "📋 Current version: v$CURRENT_VERSION" 
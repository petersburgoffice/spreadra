#!/bin/bash

# Скрипт для обновления версии плагина
# Использование: ./update_version.sh 0.3.4

if [ $# -eq 0 ]; then
    echo "Использование: $0 <новая_версия>"
    echo "Например: $0 0.3.4"
    exit 1
fi

NEW_VERSION=$1

# Проверяем формат версии
if [[ ! $NEW_VERSION =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Ошибка: Неправильный формат версии. Используйте формат X.Y.Z"
    exit 1
fi

# Читаем текущую версию
if [ -f "version.txt" ]; then
    OLD_VERSION=$(cat version.txt)
else
    OLD_VERSION="неизвестно"
fi

echo "Обновляю версию: $OLD_VERSION -> $NEW_VERSION"

# Записываем новую версию
echo "$NEW_VERSION" > version.txt

# Переконфигурируем CMake и собираем плагин
echo "Переконфигурирую CMake..."
cd build_simple && cmake .. && cd ..

echo "Компилирую плагин..."
cmake --build build_simple --target Spreadra_AU

echo "✅ Версия обновлена до $NEW_VERSION"
echo "✅ Плагин скомпилирован и установлен"

# Проверяем что версия в плагине правильная
PLUGIN_VERSION=$(strings "/Users/azverev/Library/Audio/Plug-Ins/Components/Spreadra.component/Contents/MacOS/Spreadra" | grep -E "^[0-9]+\.[0-9]+\.[0-9]+$" | head -1)

if [ "$PLUGIN_VERSION" = "$NEW_VERSION" ]; then
    echo "✅ Версия в плагине: $PLUGIN_VERSION"
else
    echo "⚠️  Версия в плагине: $PLUGIN_VERSION (ожидалась: $NEW_VERSION)"
fi 
# Makefile для Reverbix с автоматическим версионированием

.PHONY: all clean build install version

# Основная цель
all: version build install

# Увеличить версию
version:
	@echo "📈 Incrementing version..."
	@./increment_version.sh

# Собрать проект
build:
	@echo "🔨 Building project..."
	@mkdir -p build
	@cd build && cmake .. && cmake --build .
	@echo "📋 Current version: v$(shell cat version.txt)"

# Установить AU плагин
install:
	@echo "📦 Installing AU plugin..."
	@rm -rf ~/Library/Audio/Plug-Ins/Components/Reverbix.component
	@cp -R build/Reverbix_artefacts/AU/Reverbix.component ~/Library/Audio/Plug-Ins/Components/
	@echo "✅ AU plugin installed!"
	@echo "📋 Current version: v$(shell cat version.txt)"

# Быстрая сборка (без инкремента версии)
quick:
	@echo "⚡ Quick build (no version increment)..."
	@mkdir -p build
	@cd build && cmake --build .

# Очистка
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf build
	@echo "✅ Cleaned!"

# Показать текущую версию
show-version:
	@echo "📋 Current version: $(shell cat version.txt)"

# Помощь
help:
	@echo "Available targets:"
	@echo "  all          - Increment version, build and install"
	@echo "  version      - Increment version only"
	@echo "  build        - Build project only"
	@echo "  install      - Install AU plugin only"
	@echo "  quick        - Quick build without version increment"
	@echo "  clean        - Clean build directory"
	@echo "  show-version - Show current version"
	@echo "  help         - Show this help" 
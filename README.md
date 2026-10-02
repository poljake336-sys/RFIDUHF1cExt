# RFIDUHF — внешняя компонента RFID для мобильной платформы 1С

**Автор:** Каюмов А.Р.  
**Дата:** 02.10.2026

## Готовый файл для 1С

```
E:\RFIDUHF1cExt\dist\RFIDUHF.zip
```

(копия также: `RFIDUHF1cExt\dist\RFIDUHF.zip` в workspace)

Макет двоичных данных → загрузить этот zip → подключить:

```bsl
ПодключитьВнешнююКомпоненту("ОбщийМакет.RFIDUHF", "RFID", ТипВнешнейКомпоненты.Native);
RFID = Новый("AddIn.RFID.RFIDUHF");
```

Пример: `examples\МобильныйКлиент_RFID.bsl`

## Где править и собирать (важно)

| Путь | Назначение |
|------|------------|
| `E:\RFIDUHF1cExt` | **Рабочая сборка** (только ASCII — иначе NDK/clang на Windows падает) |
| `e:\RFID компанента\...` | Workspace / документы; для Gradle не использовать |

Сборка:

```bat
E:\RFIDUHF1cExt\build.cmd
```

SDK: `C:\Android\Sdk` (junction на реальный SDK).  
JDK: Android Studio JBR 21.  
Gradle 8.9 + AGP 8.7.

Старые проекты в `vendor/` / `reference/` с Gradle 5/7 **не импортируются** в IDE (см. `.vscode/settings.json`) — из‑за них были ошибки «Gradle 5.4.1 + Java 21».

## API

Методы: Инициализировать, Завершить, НачатьИнвентаризацию, ОстановитьИнвентаризацию, СчитатьОднуМетку, УстановитьМощность, УстановитьЧастотныйРежим  

События: источник `org.rfiduhf.addin`, `TagRead` (JSON epc/tid/rssi/ts), `Error`

Проверка только на ТСД с UHF (Chainway C72). На эмуляторе init вернёт Ложь.

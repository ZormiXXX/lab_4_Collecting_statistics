# Лабораторная работа №4

Реализация на C++17 для темы: **онлайн-статистика потока событий** на базе собственных `LazySequence<T>` и семейства потоков `Stream<T>`.

## Что реализовано

- `LazySequence<T>` с мемоизацией, поддержкой конечных и бесконечных последовательностей.
- Ординальный доступ к ленивым последовательностям через `Ordinal`: `n`, `omega`, `omega + n`.
- Операции `Get`, `GetFirst`, `GetLast`, `GetSubsequence`, `Append`, `Prepend`, `InsertAt`, `Concat`, `Map`, `Reduce`, `Where`, `Zip`, `Take`.
- Внутренний генератор `LazySequence<T>::Generator` для последовательного перечисления элементов.
- Семейство потоков:
  - `SequenceReadStream<T>`
  - `LazySequenceReadStream<T>`
  - `StringReadStream<T>`
  - `FileReadStream<T>`
  - `CsvReadStream<T>`
  - `JsonArrayReadStream<T>`
  - `MemoryWriteStream<T>`
  - `FileWriteStream<T>`
- `CircularBuffer<T>` для поддержки скользящего окна без хранения всей истории.
- Новые generic-модули лабораторной разнесены на объявления в `.hpp` и реализации в `.tpp`.
- Тематическая задача: онлайн-вычисление статистик потока:
  - среднее и выборочная дисперсия по формуле Уэлфорда;
  - минимум/максимум;
  - медиана в один проход через две бинарные кучи;
  - скользящее среднее по окну;
  - детектор выбросов по z-score;
  - экспорт результатов в CSV и JSON.

## Сборка и запуск

```bash
make
make run
make test
```

## Поддерживаемые источники данных

- строка чисел;
- `LazySequence<T>`;
- текстовый файл с одним значением в строке;
- `CSV`-файл;
- `JSON`-массив чисел.

## Выходные файлы

- `output/stream_dump.txt` — дамп потока из stream-демонстрации;
- `output/latest_stats.csv` — подробный CSV-лог по шагам;
- `output/latest_stats_summary.json` — итоговая JSON-сводка по последнему запуску анализа.

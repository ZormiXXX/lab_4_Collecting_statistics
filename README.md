# Лабораторная работа №4

Реализация на C++17 для темы: **онлайн-статистика потока событий** на базе собственных `LazySequence<T>` и семейства потоков `Stream<T>`.

## Что реализовано

- `LazySequence<T>` с мемоизацией, поддержкой конечных и бесконечных последовательностей.
- Операции `Get`, `GetFirst`, `GetLast`, `GetSubsequence`, `Append`, `Prepend`, `InsertAt`, `Concat`, `Map`, `Reduce`, `Where`, `Zip`, `Take`.
- Внутренний генератор `LazySequence<T>::Generator` для последовательного перечисления элементов.
- Семейство потоков:
  - `SequenceReadStream<T>`
  - `LazySequenceReadStream<T>`
  - `StringReadStream<T>`
  - `FileReadStream<T>`
  - `MemoryWriteStream<T>`
  - `FileWriteStream<T>`
- Тематическая задача: онлайн-вычисление статистик потока:
  - среднее и выборочная дисперсия по формуле Уэлфорда;
  - минимум/максимум;
  - медиана в один проход через две бинарные кучи;
  - скользящее среднее по окну;
  - детектор выбросов по z-score;
  - экспорт результатов в CSV.

## Сборка и запуск

```bash
make
make run
make test
```

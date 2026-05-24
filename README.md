# SET 9, A1: строковые сортировки

Репозиторий содержит решения сопутствующих задач Codeforces и экспериментальную инфраструктуру для задачи A1.

## Состав

- `codeforces/A1m.cpp` — String Merge Sort с LCP.
- `codeforces/A1q.cpp` — тернарный String Quick Sort.
- `codeforces/A1r.cpp` — MSD Radix Sort без переключения.
- `codeforces/A1rq.cpp` — MSD Radix Sort с переключением на String Quick Sort при размере фрагмента меньше 74.
- `src/main.cpp` — экспериментальный C++17-проект: `StringGenerator`, `StringSortTester`, шесть сортировок и счётчик посимвольных сравнений.
- `data/raw_results.csv`, `data/averaged_results.csv` — результаты реально выполненных замеров.
- `figures/*.png` — графики по CSV с подписями на русском.
- `scripts/build_report.py` — сборка локального отчёта `report/report_A1.docx`; сам `.docx` не хранится в репозитории.
- `submissions_ids.txt` — места для ID успешных посылок Codeforces.

Публичный репозиторий для отчёта: `https://github.com/Irek-dev/SET9_A.git`.

## Параметры эксперимента

- Язык: C++17.
- Seed: `20260524`.
- Алфавит: `A..Z`, `a..z`, `0..9`, `!@#%:;^&*()-` — всего 74 символа.
- Длина строк: от 10 до 200.
- Размеры массивов: от 100 до 3000 с шагом 100.
- Типы массивов: случайные, обратно отсортированные, почти отсортированные.
- Повторов: 10 для каждой комбинации.
- Для обычного Quick Sort и String Quick Sort опорный элемент выбирается одинаково: середина текущего фрагмента.

## Команды

Для графиков и отчёта нужен Python с пакетами `pillow`, `python-docx` и `pandas`:

```bash
PYTHON=python3
```

Компиляция и тестирование решений Codeforces:

```bash
python3 scripts/run_tests.py
```

Сборка и запуск полного эксперимента:

```bash
clang++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  src/main.cpp \
  -o build/string_sort_experiment

./build/string_sort_experiment --runs 10 --seed 20260524 \
  --raw data/raw_results.csv --averaged data/averaged_results.csv
```

Построение графиков:

```bash
$PYTHON scripts/plot_results.py
```

Сборка отчёта:

```bash
$PYTHON scripts/build_report.py
$PYTHON scripts/check_docx.py report/report_A1.docx
```

## Инструкция по сдаче

1. Отправить `codeforces/A1m.cpp`, `A1q.cpp`, `A1r.cpp`, `A1rq.cpp` в соответствующие задачи Codeforces.
2. ID успешных отправок внесены в `submissions_ids.txt`, `README.md` и отчёт.
3. Загрузить локальный `report/report_A1.docx` в LMS. Внутри отчёта указана ссылка на публичный репозиторий с исходниками и CSV.

## ID посылок

- A1m: 375971555
- A1q: 375971572
- A1r: 375971594
- A1rq: 375971618

# Performance Evaluation of a Single/Multi core

## How to Run

1. Build the project:

```bash
make
```

2. Create and activate a Python virtual environment:

```bash
python3 -m venv cpa
source cpa/bin/activate
```

3. Install dependencies:

```bash
pip install -r benchmark/requirements.txt
```

4. Run the benchmark script:
```bash
python3 benchmark/benchmark.py
```
to run all benchmarks, or
```bash
python3 benchmark/benchmark.py --op <option_num>
```
to run only one type of multiplication.
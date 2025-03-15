# Run Commands for the Graph Drawing Project

```bash
python cleaner.py data/<FILENAME>.mtx graph.txt
```
# Run g++ for compile one time
```bash
g++ main.cpp -I/path/to/eigen -o spectral_embed -O2
```
# Then run this command
```bash
./spectral_embed graph.txt 1 1 3
python draw.py
```
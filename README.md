# Ukázka práce s OneWire čidlem
Navazuje na zadání práce TOI. Postup pro přidání komponent do vlastního projektu

```bash
mkdir components
cd components
git clone https://github.com/DavidAntliff/esp32-owb.git
git clone https://github.com/DavidAntliff/esp32-ds18b20.git
```

V tomto případě jsou vložené jako submoduly, takže pro tento projekt bude stačit spustit
```bash
git submodule update --init --recursive 
```

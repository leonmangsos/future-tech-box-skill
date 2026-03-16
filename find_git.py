import os
import glob

paths = [
    r"C:\Program Files\Git\cmd\git.exe",
    r"C:\Program Files\Git\bin\git.exe",
    r"C:\Program Files (x86)\Git\cmd\git.exe",
    r"C:\Program Files (x86)\Git\bin\git.exe",
]

# 也搜索常见位置
search_paths = glob.glob(r"C:\Program Files*\Git*\*\git.exe")
paths.extend(search_paths)

for p in paths:
    if os.path.exists(p):
        print(f"Found: {p}")
        break
else:
    print("Git not found in standard locations")

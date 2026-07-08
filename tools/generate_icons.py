"""从 app.ico 生成 Android mipmap 图标。用法: python generate_icons.py <ico_path> <android_res_dir>"""

import sys, os
from PIL import Image

DENSITIES = {
    'mdpi': 48, 'hdpi': 72, 'xhdpi': 96,
    'xxhdpi': 144, 'xxxhdpi': 192,
}

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <ico_path> <android_res_dir>")
        sys.exit(1)

    ico_path = sys.argv[1]
    res_dir = sys.argv[2]

    if not os.path.exists(ico_path):
        print(f"Error: {ico_path} not found")
        sys.exit(1)

    img = Image.open(ico_path)
    for density, size in DENSITIES.items():
        out_dir = os.path.join(res_dir, f"mipmap-{density}")
        os.makedirs(out_dir, exist_ok=True)
        out_path = os.path.join(out_dir, "ic_launcher.png")
        resized = img.resize((size, size), Image.LANCZOS)
        if resized.mode != 'RGBA':
            resized = resized.convert('RGBA')
        resized.save(out_path, 'PNG')
        print(f"  {density}: {size}x{size}")

if __name__ == '__main__':
    main()

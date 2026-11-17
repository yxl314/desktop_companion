#!/usr/bin/env python3
"""生成占位桌宠形象（透明 PNG）。原型阶段使用，后续可替换为正式素材。"""
from PIL import Image, ImageDraw

S = 4  # 超采样倍数
W, H = 240 * S, 320 * S
img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

BODY = (255, 233, 192, 255)
BODY_LINE = (226, 199, 150, 255)
CREAM = (255, 246, 224, 255)
DARK = (74, 59, 46, 255)
BLUSH = (255, 170, 165, 150)

# 身体
d.ellipse((30 * S, 78 * S, 210 * S, 312 * S), fill=BODY, outline=BODY_LINE, width=3 * S)
# 耳朵
d.ellipse((60 * S, 40 * S, 96 * S, 96 * S), fill=BODY, outline=BODY_LINE, width=3 * S)
d.ellipse((144 * S, 40 * S, 180 * S, 96 * S), fill=BODY, outline=BODY_LINE, width=3 * S)
# 肚皮
d.ellipse((78 * S, 190 * S, 162 * S, 296 * S), fill=CREAM)
# 眼睛
d.ellipse((92 * S, 148 * S, 114 * S, 180 * S), fill=DARK)
d.ellipse((126 * S, 148 * S, 148 * S, 180 * S), fill=DARK)
# 高光
d.ellipse((97 * S, 153 * S, 104 * S, 160 * S), fill=(255, 255, 255, 255))
d.ellipse((131 * S, 153 * S, 138 * S, 160 * S), fill=(255, 255, 255, 255))
# 腮红
d.ellipse((66 * S, 186 * S, 96 * S, 202 * S), fill=BLUSH)
d.ellipse((144 * S, 186 * S, 174 * S, 202 * S), fill=BLUSH)
# 嘴
d.arc((112 * S, 178 * S, 128 * S, 196 * S), start=20, end=160, fill=DARK, width=2 * S)
# 脚
d.ellipse((78 * S, 288 * S, 112 * S, 314 * S), fill=BODY, outline=BODY_LINE, width=3 * S)
d.ellipse((128 * S, 288 * S, 162 * S, 314 * S), fill=BODY, outline=BODY_LINE, width=3 * S)
# 头顶呆毛
d.line((120 * S, 44 * S, 120 * S, 20 * S), fill=BODY_LINE, width=4 * S)

img = img.resize((240, 320), Image.LANCZOS)
img.save("resources/pet.png")
print("saved resources/pet.png")

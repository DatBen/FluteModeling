from PIL import Image


height,width=100,200
X_offset=5

img = Image.new('RGB', (width, height), color = 'white')

w_bouche=15
h_bouche=35  


w_bec=20
h_bec=h_bouche-2*w_bouche

w_res=50
h_res=15

h_sortie=5
w_sortie=h_sortie

trou_bizot=6
offset_bizot=h_bec//2
w_bizot=4

def create_bouche():
    for i in range (h_bouche//2):
        img.putpixel((X_offset,height//2+i),(0,0,0))
        img.putpixel((X_offset,height//2-i),(0,0,0))
        if i<(h_bouche//2-1):
            img.putpixel((X_offset+1,height//2+i),(255,0,0))
            img.putpixel((X_offset+1,height//2-i),(255,0,0))
    img.putpixel((X_offset+1,height//2+h_bouche//2-1),(0,0,0))
    img.putpixel((X_offset+1,height//2-h_bouche//2+1),(0,0,0))
    img.putpixel((X_offset+2,height//2+h_bouche//2-1),(0,0,0))
    img.putpixel((X_offset+2,height//2-h_bouche//2+1),(0,0,0))
    
    for i in range(1,w_bouche):
        img.putpixel((X_offset+i+2,height//2-i+h_bouche//2),(0,0,0))
        img.putpixel((X_offset+i+2,height//2-h_bouche//2+i),(0,0,0))
        img.putpixel((X_offset+i+3,height//2-i+h_bouche//2),(0,0,0))
        img.putpixel((X_offset+i+3,height//2-h_bouche//2+i),(0,0,0))

def create_bec():
    offset=X_offset+2+w_bouche
    for i in range (w_bec):
        img.putpixel((offset+i,height//2+h_bouche//2-w_bouche),(0,0,0))
        img.putpixel((offset+i,height//2-(h_bouche//2-w_bouche)),(0,0,0))
    img.putpixel((offset+w_bec,height//2+h_bouche//2-w_bouche+1),(0,0,0))
    img.putpixel((offset+w_bec,height//2-(h_bouche//2-w_bouche+1)),(0,0,0))
    img.putpixel((offset+w_bec,height//2-(h_bouche//2-w_bouche+2)),(0,0,0))
    img.putpixel((offset+w_bec-1,height//2+h_bouche//2-w_bouche+1),(0,0,0))
    img.putpixel((offset+w_bec-1,height//2-(h_bouche//2-w_bouche+1)),(0,0,0))

def create_res():
    offset=X_offset+w_bouche+w_bec+2
    for i in range(w_res):
        img.putpixel((offset+trou_bizot+i,height//2-offset_bizot),(0,0,0))
    for i in range(h_res-h_bec//2-2+offset_bizot):
        img.putpixel((offset,height//2+h_bec-1+i),(0,0,0))
    for i in range(0,w_res+trou_bizot):
        img.putpixel((offset+i,height//2+h_res+offset_bizot),(0,0,0))
    for i in range(w_sortie):

        img.putpixel((offset+i+w_res+trou_bizot,height//2+i-offset_bizot+1),(0,0,0))
        img.putpixel((offset+i+w_res+trou_bizot,height//2+h_res+offset_bizot-1-i),(0,0,0))
        img.putpixel((offset+i+w_res+trou_bizot,height//2+i-offset_bizot),(0,0,0))
        img.putpixel((offset+i+w_res+trou_bizot,height//2+h_res+offset_bizot-i),(0,0,0))


def create_bizot():
    offset=X_offset+w_bouche+w_bec+trou_bizot+2
    for i in range(1,w_bizot):
        for j in range(w_res-i):
            img.putpixel((offset+i+j,height//2-offset_bizot-i),(0,0,0))








create_bouche()
create_bec()
create_res()
create_bizot()

img.save('test.png')
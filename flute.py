import imageio

image = imageio.read("flute.png")
data = image.get_data(0)

f = open("flute.h","w")
taille = data.shape
f.write("#define SIZE_X %s\n" % taille[1] )
f.write("#define SIZE_Y %s\n" % taille[0])

noirs = []
rouges = []
for y in range(len(data)):
    for x in range(len(data[0])):
        p = data[y][x]
        if sum(p) <= 10:
            noirs.append(taille[1]*y+x)
        elif p[0] >= 200 and p[1] <50 and p[2] < 50:
            rouges.append(taille[1]*y+x)
            #print("%x,%x,%x" % (x,y,p[0]))
f.write("int nb_wall = %s;\n" % len(noirs)) 
f.write("int wall[] = {") 
f.write(", ".join([str(x) for x in noirs]))
f.write("};\n")  
f.write("int nb_blow = %s;\n" % len(rouges))
f.write("int blow[]={%s};\n" % ",".join([str(x) for x in rouges]))
f.close()

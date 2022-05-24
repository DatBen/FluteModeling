import imageio

image = imageio.read("test2.png")
data = image.get_data(0)

f = open("include/flute.h","w")
g = open("flute.cl","w")
taille = data.shape
f.write("#define SIZE_X %s\n" % taille[1] )
f.write("#define SIZE_Y %s\n" % taille[0])
g.write("#define SIZE_X %s\n" % taille[1] )
g.write("#define SIZE_Y %s\n" % taille[0])
g.write("#define LATICE_D 2 \n")
g.write("#define LATICE_Q 9\n")
g.write("#define NUMEL %s\n" % (taille[0]*taille[1]))


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
g.write("__constant int nb_wall = %s;\n" % len(noirs)) 
g.write("__constant int wall[] = {") 
g.write(", ".join([str(x) for x in noirs]))
g.write("};\n")  
g.write("__constant int nb_blow = %s;\n" % len(rouges))
g.write("__constant int blow[]={%s};\n" % ",".join([str(x) for x in rouges]))
g.write("__constant int dir[3] = {1,5,8}\n")
g.write("__constant int dir_fs[3] = {3,6,7}\n")

g.write("\n\n");
g.close()

with open('flute.cl', 'a') as output, open('flute_template.txt', 'r') as input:
    output.write(input.read())

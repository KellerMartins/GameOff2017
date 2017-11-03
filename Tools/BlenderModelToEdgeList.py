import bpy

filePath = 'D:\Documents\Keller\Projetos\GameJams\GameOff2017\Models\Cube.txt'
f = open(filePath, 'w')
obdata = bpy.context.object.data

vCount = 0;
for v in obdata.vertices:
    vCount+=1
f.write('{} '.format(vCount))

for v in obdata.vertices:
    f.write('{} {} {} '.format(v.co.x, v.co.y, v.co.z))

eCount = 0;
for e in obdata.edges:
    eCount+=1
f.write('{} '.format(eCount))

for e in obdata.edges:
    f.write('{} {} '.format(e.vertices[0], e.vertices[1]))

print(obdata.name + " exported sucessfully!")
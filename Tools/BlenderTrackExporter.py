import bpy
import math

folderPath = "..\Tracks\\"
fileName = "TestTrack"

#Export model
filePath = folderPath+fileName+"Model.txt"
f = open(filePath, 'w')
ob = bpy.data.objects['Model']

vCount = 0;
for v in ob.data.vertices:
    vCount+=1
f.write('{} '.format(vCount))

for v in ob.data.vertices:
    f.write('{} {} {} '.format(v.co.x, v.co.y, v.co.z))

eCount = 0;
for e in ob.data.edges:
    eCount+=1
f.write('{} '.format(eCount))

for e in ob.data.edges:
    f.write('{} {} '.format(e.vertices[0], e.vertices[1]))

f.close()


#Export Path
filePath = folderPath+fileName+"Path.txt"
f = open(filePath, 'w')
ob = bpy.data.objects['Path']

vCount = 0;
for v in ob.data.vertices:
    vCount+=1
f.write('{} '.format(vCount))

for v in ob.data.vertices:
    f.write('{} {} {} '.format(v.co.x, v.co.y, v.co.z))

eCount = 0;
for e in ob.data.edges:
    eCount+=1
f.write('{} '.format(eCount))

for e in ob.data.edges:
    f.write('{} {} '.format(e.vertices[0], e.vertices[1]))

f.close()

#Export Track Data
filePath = folderPath+fileName+"Data.txt"
f = open(filePath, 'w')

#Track width
f.write('10.117 ')

#Start line
ob = bpy.data.objects['Start']
f.write('{} {} {} '.format(ob.location.x, ob.location.y, ob.location.z))

#Car Spawns
for n in range(0,6):
    ob = bpy.data.objects['Spawn_{}'.format(n)]
    f.write('{} {} {} '.format(ob.location.x, ob.location.z, ob.location.y))
    f.write('{} {} {} '.format(math.degrees(ob.rotation_euler.x), math.degrees(ob.rotation_euler.z), math.degrees(ob.rotation_euler.y)))

f.close()
print("Track " + fileName + " exported sucessfully!")
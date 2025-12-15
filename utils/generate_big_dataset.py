import sys, random

outf = "data/bigdata.csv"

if len(sys.argv) < 2 or not sys.argv[1].isnumeric() and int(sys.argv[1]) <= 0:
    print("Error: You have to pass a valid positive number of lines") 

lines= int( sys.argv[1] )
cols = ['name', 'surname', 'age', 'gender', 'height']

with open(outf, "w") as f:
    f.write(','.join(cols) + "\n")
    for i in range(lines):
        name = chr(random.randint(65, 80)) * 10
        surname = chr(random.randint(65, 80)) * 8
        age = random.randint(10, 80)
        gender= random.choice(['f', 'm'])
        height = random.randint(100, 200) / 100.0
        f.write(f"{name},{surname},{age},{gender},{height}\n")

with open(outf, 'rb') as f:
    f.seek(0, 2 )
    size = f.tell()
    print(f"File size: {size} bytes")
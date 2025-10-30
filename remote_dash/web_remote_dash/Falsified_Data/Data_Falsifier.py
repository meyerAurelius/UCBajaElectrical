import json, random, math, time

while True:

    x = random.random()
    y = random.random()
    z = random.random()


    with open ("DATA_ACCESS/Current_acceleration.json", "w") as file:
        f = json.dumps({"Xaccel": x, "Yaccel": y, "Zaccel": z})
        file.write(f)

    time.sleep(1)

    with open ("DATA_ACCESS/Current_acceleration.json", "r") as file:
        print(file.read())
    if x < 0.01:
        break


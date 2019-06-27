import re
from functools import reduce
from collections import defaultdict

file_name = "test2"

data = ""
with open(file_name + '.txt', 'r') as file:
    data = file.read()

# "D test\n hi\nD 1\nD x"

pattern = r'(?:^|\n)((?:D|E) \(\d+\)) EspNow Wrapper:([^\n]*)'
x = re.findall(pattern, data)

pass1 = reduce((lambda x, y: x + y[0] + y[1] + "\n"), [""] + x)
# print(pass1)


# send = r'(?:Sending packet (?P<send>\d+))'
# recv = r'(?:Got packet (?P<recv>\d+))'
# rssi = r'(?:Recived packet with RSSI: (?P<RSSI>(?:\-?)\d+) from (?P<mac>(?:[a-f]|[0-9]|\:)+))'
# fail = r'(?:Failed to send to mac: (?P<macf>(?:[a-f]|[0-9]|\:)+))'
# comb = r'(?:' + send + r'|' + recv + r'|' + rssi + r'|' + fail + r')'
# pattern2 = r'(?:^|\n)(?:(?:D|E) \((\d+)\) ' + comb + '\n)'
pattern2 = r'(?:^|\n)(?:(?:D|E) \((\d+)\) (?:(?:Sending packet (?P<send>\d+))|(?:Got packet (?P<recv>\d+))|(?:Recived packet with RSSI: (?P<RSSI>(?:\-?)\d+) from (?P<mac>(?:[a-f]|[0-9]|\:)+))|(?:Got marker return (?P<mark>\d+))|(?:Failed to send to mac: (?P<macf>(?:[a-f]|[0-9]|\:)+))))'

x2 = re.findall(pattern2, pass1)
# print(x2)
# print(pattern2)

f = open(file_name + "_rssi_data.csv", "w")
x = [[]]
y = [[]]
m = 0
markers = [0]

# for i in x2:
#     if i[3] != '':
#         f.write(i[0] + "," + i[3] + "\n")
#     if i[5] != '':
#         f.write("Marker," + i[5] + "\n")



for i in x2:
    if i[3] != '':
        x[m].append(int(i[0]))
        y[m].append(int(i[3]))
        # f.write(i[0] + "," + i[3] + "\n")
    if i[5] != '':
        # f.write("Marker," + i[5] + "\n")
        m = m + 1
        markers.append(i[5])
        x.append([])
        y.append([])

max_len = max(map(len, x))

for i in markers:
    f.write(str( (int(i) - 1)*2.5 ) + ",,")
f.write("\n")

for i in range(0, max_len):
    for j in range(0, len(x)):
        if (len(x[j]) > i):
            f.write(str(x[j][i]) + ", " + str(y[j][i]) + ",")
        else:
            f.write(",,")
    f.write("\n")

f.close()

vals = defaultdict(lambda: (0, 0, False))
m = 0

for i in x2:
    if i[1] != '':
        # f.write("Marker," + i[5] + "\n")
        # m = m + 1
        # markers.append(i[5])
        # x.append([])
        # y.append([])
        n = int(i[1])
        vals[n] = (m, n, False)
    elif i[2] != '':
        n = int(i[2])
        old = vals[n]
        vals[n] = (old[0], n, True)
    elif i[5] != '':
        m = m + 1

print(vals)

totals = [0 for x in range(0, m+1)]
dropped = [0 for x in range(0, m+1)]

for i in vals:
    j = vals[i]
    totals[j[0]] += 1
    if not j[2]:
        dropped[j[0]] += 1



vals = [dropped[i]/totals[i] for i in range(1, m)]
print(vals)
print(totals)
print(dropped)
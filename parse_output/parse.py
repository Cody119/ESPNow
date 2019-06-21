import re
from functools import reduce

data = ""
with open('out.txt', 'r') as file:
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

f = open("rssi_data.csv", "w")
p = {}
m = 0

for i in x2:
    if i[3] != '':
        f.write(i[0] + "," + i[3] + "\n")
    if i[5] != '':
        f.write("Marker," + i[5] + "\n")

f.close()

print(len(x))
print(len(x2))


# D (22843) Sending packet 45
# D (22843) Recived packet with RSSI: -41 from 30:ae:a4:0d:70:a4
# D (22843) Got packet 44
# Failed to send to mac: 30:ae:a4:0d:70:a4
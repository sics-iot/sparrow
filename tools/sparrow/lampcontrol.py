import tlvlib, sys, struct, socket, binascii

if len(sys.argv) < 2:
    print "Usage ", sys.argv[0], " <host> <intensity in percent>"
    exit(1)

host = sys.argv[1]
p = float(sys.argv[2])
if p > 100:
    print "Too high value 0 - 100"
    exit(1)

print "Setting lamp to ", p
d = tlvlib.discovery(host)

i = 1
for data in d[1]:
    if data[0] == tlvlib.INSTANCE_LAMP:
        print "Found LAMP instnace, setting intensity."
        t1 = tlvlib.create_set_tlv32(i, 0x100, (p * 0.01) * 0xffffffff)
        enc,tlvs = tlvlib.send_tlv([t1], host)
    i = i + 1

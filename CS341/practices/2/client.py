import argparse
import socket
import struct


def split_bytes(bs):
    # split the given number or byte sequence into chunks consisting of 4-byte numbers (or less if there is not enough bytes)
    # 0x0f1234ce (or b"\x0f\x12\x34\xce") => [0x0f12, 0x34ce]
    ls = []
    n = bs
    if type(n) is not int:
        n = int.from_bytes(bs, byteorder='big')
    while n != 0:
        ls.insert(0, n & 0xffff)
        n >>= 16
    return ls


def one_complement_sum(ls):
    # do the one's complement sum of the list
    # sum the list, and repeatedly carrying down the overflowed amount
    s = sum(ls)
    while s != (s & 0xffff):
        s = (s & 0xffff) + (s >> 16)
    return s


def zero_padding(len):
    # make the zero-byte padding to make the length of data (`len`) a multiple of 4
    if len % 4 == 0:
        return b""
    return b"\x00" * (4 - (len % 4))


def change_endianness(n):
    # change big endian to little endian and vice versa
    assert n == (n & 0xffff)
    return ((n & 0xff00) >> 8) | ((n & 0xff) << 8)


def get_checksum(flag, keyword, sid, length, data):
    # split the message and data (with proper zero padding),
    # sum them, get the 1's complement by XOR with 0xffff,
    # and finally change the endianness
    bytes_array = split_bytes(struct.pack(
        '!H4sII', flag, keyword, sid, length) + data + zero_padding(len(data)))
    return change_endianness(one_complement_sum(bytes_array) ^ 0xffff)


def pack_data(flag, keyword, sid, data):
    # pack the data into bytes with given flag, keyword, and sid
    length = 16 + len(data)
    checksum = get_checksum(flag, keyword, sid, length, data)
    return struct.pack(
        '!HH4sII', flag, checksum, keyword, sid, length) + data


def en_or_decrypt(key, data):
    # do the XOR cipher, by repeatedly applying `en_or_decrypt_four_bytes` below to each 4 bytes
    postproc = ""
    while len(postproc) < len(data):
        postproc += en_or_decrypt_four_bytes(key,
                                             data[len(postproc):len(postproc)+4])

    return postproc


def en_or_decrypt_four_bytes(key, data):
    # make the key and the data into int's,
    # do XOR, and re-pack into bytes.
    # if the length of `data` is less than 4,
    # cut the result accordingly (to make the result has the same length as `data`)
    unpacked_key = struct.unpack("!I", key)[0]
    four_bytes = struct.unpack("!I", (data + b"\x00\x00\x00")[0:4])[0]

    postproc = struct.pack("!I", unpacked_key ^ four_bytes).decode()
    return postproc[0:len(data)]


def run(host, port, sid):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # connect to the given host and port
        s.connect((host, port))

        # make and send the initial packet
        initial_packet = pack_data(1, b"sbmt", sid, b"")
        s.send(initial_packet)

        while True:
            msg = s.recv(10000)

            # if the header is arrived incompletely, wait again
            if len(msg) < 16:
                continue

            # get the complete header
            (flag, checksum, keyword, sid_, length) = struct.unpack(
                '!HH4sII', msg[0:16])

            # receive until the claimed length is reached
            while len(msg) < length:
                msg += s.recv(length - len(msg))

            # the data section
            data = msg[16:]

            # assert the checksum matches
            assert get_checksum(flag, keyword, sid_, length, data) == checksum

            # if the message indicates the scoring
            if keyword == b"FAIL" or keyword == b"GOOD":
                print("KEY:", keyword.decode())
                print("Score:", sid_)
                print("Message:", msg[16:].decode())
                break

            # decrypt the message
            decrypted = en_or_decrypt(keyword, data)

            # make a packet and send it
            packet = pack_data(flag, keyword, sid_, decrypted.encode())
            s.send(packet)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="python client.py --host=143.248.56.39 --port=4000 --studentID=20xxxxxx")
    parser.add_argument('--host', help="host ip", required=True)
    parser.add_argument('--port', help="port number", required=True)
    parser.add_argument('--studentID', help="student id", required=True)

    args = parser.parse_args()
    run(host=args.host, port=int(args.port), sid=int(args.studentID))

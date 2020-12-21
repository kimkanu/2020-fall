import argparse
import socket
import struct
import uuid
import threading
import time

MAX_TEXT_LEN = 100
MAX_BYTES = 10000

#             reset,        red,     yellow,      green,       blue, none
COLORS = ["\033[0m", "\033[95m", "\033[93m", "\033[92m", "\033[94m", ""]

# Lock the print function for multi-thread printing
print_lock = threading.Lock()


def thread_print(color, msg):
    # Thread-safe print function
    with print_lock:
        print(COLORS[color] + msg + COLORS[0])


def ellipse(s):
    # Cut a text which is pretty long
    if len(s) <= MAX_TEXT_LEN:
        return s
    return s[0:MAX_TEXT_LEN - 3] + "..."


def answer(ans, uid):
    # Transform into the answer form
    resp = str(ans) + "_" + str(uid)
    return resp.encode()


def fib(n, mem={0: 0, 1: 1}):
    # Obtain the n-th Fibonacci n with memoization
    if n not in mem:
        # F_n = F_{n - 1} + F_{n - 2}
        mem[n] = fib(n - 1, mem) + fib(n - 2, mem)
    return mem[n]


def fact(n, mem={0: 1}):
    # Seek n! with memoization
    if n not in mem:
        mem[n] = n * fact(n - 1, mem)
    return mem[n]


def thread_function(connection, client_address, name, uid, index):
    # The function which threads will execute
    # Set the timeout
    connection.settimeout(120)

    # Send the uuid of the client
    connection.send(str(uid).encode())
    
    # Repeat the following:
    while True:
        # Get the message type: one of FIBONACCI, FACTORIAL, FILE, and COMPLETE
        msg_type = connection.recv(MAX_BYTES).decode()
        # and print it
        thread_print(index, msg_type)

        if msg_type == "FIBONACCI":
            # Get n from the client message
            n = int(connection.recv(MAX_BYTES).decode())
            thread_print(index, "[%s:%d] Fibonacci request from client, fib of %d" %
                         (client_address[0], client_address[1], n))

            # Send the answer
            connection.send(answer(fib(n), uid))
        elif msg_type == "FACTORIAL":
            # Get n from the client message
            n = int(connection.recv(MAX_BYTES).decode())
            thread_print(index, "[%s:%d] Factorial request from client, fact of %d" %
                         (client_address[0], client_address[1], n))

            # Send the answer
            connection.send(answer(fact(n), uid))
        elif msg_type == "FILE":
            # Get file length from the client message
            file_length = int(connection.recv(MAX_BYTES).decode())

            # Repeat to receive the client message
            # until the text has reached the claimed file length
            current_text = ""
            while len(current_text) < file_length:
                text = connection.recv(file_length).decode()
                current_text += text
            # and print it (with trimming if it is too long)
            thread_print(index, ellipse(current_text))

            # The number of words in the text
            ans = len(current_text.split(" "))
            thread_print(index, "[%s:%d] File request from client, words in file are %d" %
                         (client_address[0], client_address[1], ans))

            # Send the answer
            connection.send(answer(ans, uid))
        elif msg_type == "COMPLETE":
            connection.close()
            break


def run(host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        # Bind the socket to the given host and port
        s.bind((host, port))

        s.listen()

        while True:
            # Accept the client connection
            connection, client_address = s.accept()

            # Receive the name of the client
            name = connection.recv(10000)
            name = name.decode()

            # The index of the client (e.g., 1 for Client1, 2 for Client2, etc.)
            try:
                index = int(name[6:])
            except ValueError:
                index = 6

            thread_print(index, "Client name is: " + name)
            uid = uuid.uuid3(uuid.NAMESPACE_DNS, name)

            x = threading.Thread(target=thread_function, args=(
                connection, client_address, name, uid, index))
            x.start()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="python server.py --port=1234")
    parser.add_argument("--port", help="port n", required=True)

    args = parser.parse_args()
    run(host="localhost", port=int(args.port))

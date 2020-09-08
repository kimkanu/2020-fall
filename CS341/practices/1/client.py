import sys
import requests

url = sys.argv[1]
name = sys.argv[2]
number = int(sys.argv[3])

hello_world_response = requests.get(url + '/hello_world')
print(hello_world_response.json().get('message'))

hash_response = requests.get(url + '/hash', params={'name': name})
hash = hash_response.json().get('result')
print(hash)

while number != 1:
    collatz_response = requests.post(
        url + '/collatz', data={'name': name, 'hash': hash, 'number': number})
    collatz_response_dict = collatz_response.json()
    if 'error' in collatz_response_dict:
        raise Exception('ERROR: ' + collatz_response_dict.get('error'))

    number = collatz_response_dict.get('result')
    print(number)

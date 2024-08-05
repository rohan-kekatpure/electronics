import requests

BASE_URL = 'http://192.168.1.67:5000'
def test_led_state_post():
    url = f'{BASE_URL}/LED'
    payload = {'led_state': 0.75}
    response = requests.post(url, json=payload)
    print(response.json())

def test_led_state_get():
    url = f'{BASE_URL}/LED'
    response = requests.get(url)
    print(response.json())

if __name__ == '__main__':
    test_led_state_get()
    test_led_state_post()


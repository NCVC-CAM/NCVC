import requests

r = requests.get('https://api.github.com/repos/NCVC-CAM/NCVC/releases')

## リリース昇順にしないと見にくい
for item in sorted(r.json(), key=lambda k: k['name']):
    print("ReleaseName:", item["name"], item["created_at"].split("T")[0])
    for detail in item["assets"]:
        print("  ", detail["name"], end='')
        print(" download_count=", detail["download_count"])
    print("")

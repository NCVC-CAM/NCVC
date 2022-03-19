import requests

r = requests.get('https://api.github.com/repos/NCVC-CAM/NCVC/releases')
for item in r.json():
    print("ReleaseName: ", item["name"])
    for detail in item["assets"]:
        print(" FileName: ", detail["name"], end='')
        print(" download count: ", detail["download_count"])
    print("")

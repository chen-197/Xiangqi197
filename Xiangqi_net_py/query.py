import requests
import random
import sys
try:
    header={'User-Agent':'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36','Referer': 'http://www.chessdb.cn/',}
    link = "http://www.chessdb.cn/chessdb.php?action=queryall&board=" + sys.argv[1] + "&showall=1"
    r = requests.get(link, headers=header)
    html = r.content
    html = html.decode("utf-8")
    abc = html.split('|')
    k = random.randint(1,2)
    if(k == 1):
        print(abc[0])
    elif(len(abc) > int(sys.argv[2])):
        print(abc[random.randint(1,int(sys.argv[2]))])
    else:
        print(abc[random.randint(0,len(abc) - 1)])
except:
    print("ERROR")

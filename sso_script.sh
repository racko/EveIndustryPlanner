#! /bin/bash
set -euxo pipefail
#scope=publicData
scope="esi-universe.read_structures.v1%20esi-skills.read_skills.v1%20esi-characters.read_blueprints.v1%20esi-assets.read_assets.v1"
rm -f cookies.txt challenge_output.txt login_output.html headers_with_access_token.txt
data1="/oauth/authorize/?response_type=token&client_id=6207584877514be8b23ab78a6163bb82&scope=${scope}&redirect_uri=https://racko.pw:51230"
curl -Lv --cookie nada --cookie-jar cookies.txt 'https://login.eveonline.com'$data1 | tee login_output.html
data2=$(sed -n 's/.*<form action="\([^"]*\)".*/\1/p' login_output.html)
#data2="/account/logon?ReturnUrl=%2Foauth%2Fauthorize%2F%3Fresponse_type%3Dtoken%26client_id%3D6207584877514be8b23ab78a6163bb82%26scope%3D${scope}%26redirect_uri%3Dhttps%3A%2F%2Fracko.pw%3A51230"
requestVerificationToken=$(sed -n 's/.*<input name="__RequestVerificationToken" type="hidden" value="\([^"]*\)" \/>.*/\1/p' login_output.html)
echo
echo RequestVerificationToken: $requestVerificationToken
test -z $requestVerificationToken && exit 1
curl -Lv --cookie cookies.txt --cookie-jar cookies.txt --data 'UserName=drunkenmasta1&Password=XxdzqdzEuQX5Gyrx0q68&__RequestVerificationToken='$requestVerificationToken 'https://login.eveonline.com'$data2 | tee before_challenge_output.html
grep Challenge before_challenge_output.html || exit 1
echo
echo -n "Verification Code: "
read CHALLENGE
#data3="/Account/Authenticator?ReturnUrl=%2Foauth%2Fauthorize%2F%3Fresponse_type%3Dtoken%26client_id%3D6207584877514be8b23ab78a6163bb82%26scope%3D${scope}%26redirect_uri%3Dhttps%3A%2F%2Fracko.pw%3A51230"
data3=$(sed -n 's/.*<form action="\([^"]*\)".*/\1/p' before_challenge_output.html)
curl -Lv --cookie cookies.txt --cookie-jar cookies.txt  --data 'Challenge='$CHALLENGE 'https://login.eveonline.com'$data3 | tee challenge_output.html
requestVerificationToken=$(sed -n 's/.*<input name="__RequestVerificationToken" type="hidden" value="\([^"]*\)" \/>.*/\1/p' challenge_output.html)
echo
echo RequestVerificationToken: $requestVerificationToken
test -z $requestVerificationToken && exit 1
#data4="/oauth/authorize/?response_type=token&client_id=6207584877514be8b23ab78a6163bb82&scope=${scope}&redirect_uri=https://racko.pw:51230"
data4=$(sed -n 's/.*<form action="\([^"]*\)".*/\1/p' challenge_output.html)
data5="ClientIdentifier=6207584877514be8b23ab78a6163bb82&RedirectUri=https://racko.pw:51230&State=&Scope=${scope}&ResponseType=token&__RequestVerificationToken="$requestVerificationToken'&CharacterId=1679728731'
curl -D- -o/dev/null -v --cookie cookies.txt --cookie-jar cookies.txt --data $data5 'https://login.eveonline.com'$data4 | tee headers_with_access_token.txt
access_token=$(sed -n 's/.*#access_token=\([^&]*\)&.*/\1/p' headers_with_access_token.txt)
echo access_token: $access_token

#jq '.items[].type.id' ~/btsync/git/emdr/prices.json | xargs -n1 -I TYPEID -P10 curl -o /tmp/orders/buy/TYPEID.json -s -H 'Authorization: Bearer '$access_token 'https://crest-tq.eveonline.com/market/10000002/orders/buy/?type=https://public-crest.eveonline.com/types/'TYPEID'/'
#jq '.items[].type.id' ~/btsync/git/emdr/prices.json | grep -Fvx -f <(ls /tmp/orders/buy | grep -Eo '^[^.]+') |  xargs -n1 -I TYPEID curl -o /tmp/orders/buy/TYPEID.json -s -H 'Authorization: Bearer '$access_token 'https://crest-tq.eveonline.com/market/10000002/orders/buy/?type=https://public-crest.eveonline.com/types/'TYPEID'/'
#jq '.items[].type.id' ~/btsync/git/emdr/prices.json | grep -Fvx -f <(ls /tmp/orders/buy | grep -Eo '^[^.]+')
#grep -i error /tmp/orders/buy/*
#
#jq '.items[].type.id' ~/btsync/git/emdr/prices.json | xargs -n1 -I TYPEID -P10 curl -o /tmp/orders/sell/TYPEID.json -s -H 'Authorization: Bearer '$access_token 'https://crest-tq.eveonline.com/market/10000002/orders/sell/?type=https://public-crest.eveonline.com/types/'TYPEID'/'
#jq '.items[].type.id' ~/btsync/git/emdr/prices.json | grep -Fvx -f <(ls /tmp/orders/sell | grep -Eo '^[^.]+') |  xargs -n1 -I TYPEID curl -o /tmp/orders/sell/TYPEID.json -s -H 'Authorization: Bearer '$access_token 'https://crest-tq.eveonline.com/market/10000002/orders/sell/?type=https://public-crest.eveonline.com/types/'TYPEID'/'
#jq '.items[].type.id' ~/btsync/git/emdr/prices.json | grep -Fvx -f <(ls /tmp/orders/sell | grep -Eo '^[^.]+')
#grep -i error /tmp/orders/sell/*

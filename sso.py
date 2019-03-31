#! /usr/bin/env python
from http.server import HTTPServer, BaseHTTPRequestHandler
import requests
import urllib
import base64
import hashlib
import secrets
from jose import jwt
from jose.exceptions import ExpiredSignatureError, JWTError, JWTClaimsError
import sys
import json

#scopes = [
#        "esi-calendar.respond_calendar_events.v1",
#        "esi-calendar.read_calendar_events.v1",
#        "esi-location.read_location.v1",
#        "esi-location.read_ship_type.v1",
#        "esi-mail.organize_mail.v1",
#        "esi-mail.read_mail.v1",
#        "esi-mail.send_mail.v1",
#        "esi-skills.read_skills.v1",
#        "esi-skills.read_skillqueue.v1",
#        "esi-wallet.read_character_wallet.v1",
#        "esi-wallet.read_corporation_wallet.v1",
#        "esi-search.search_structures.v1",
#        "esi-clones.read_clones.v1",
#        "esi-characters.read_contacts.v1",
#        "esi-universe.read_structures.v1",
#        "esi-bookmarks.read_character_bookmarks.v1",
#        "esi-killmails.read_killmails.v1",
#        "esi-corporations.read_corporation_membership.v1",
#        "esi-assets.read_assets.v1",
#        "esi-planets.manage_planets.v1",
#        "esi-fleets.read_fleet.v1",
#        "esi-fleets.write_fleet.v1",
#        "esi-ui.open_window.v1",
#        "esi-ui.write_waypoint.v1",
#        "esi-characters.write_contacts.v1",
#        "esi-fittings.read_fittings.v1",
#        "esi-fittings.write_fittings.v1",
#        "esi-markets.structure_markets.v1",
#        "esi-corporations.read_structures.v1",
#        "esi-corporations.write_structures.v1",
#        "esi-characters.read_loyalty.v1",
#        "esi-characters.read_opportunities.v1",
#        "esi-characters.read_chat_channels.v1",
#        "esi-characters.read_medals.v1",
#        "esi-characters.read_standings.v1",
#        "esi-characters.read_agents_research.v1",
#        "esi-industry.read_character_jobs.v1",
#        "esi-markets.read_character_orders.v1",
#        "esi-characters.read_blueprints.v1",
#        "esi-characters.read_corporation_roles.v1",
#        "esi-location.read_online.v1",
#        "esi-contracts.read_character_contracts.v1",
#        "esi-clones.read_implants.v1",
#        "esi-characters.read_fatigue.v1",
#        "esi-killmails.read_corporation_killmails.v1",
#        "esi-corporations.track_members.v1",
#        "esi-wallet.read_corporation_wallets.v1",
#        "esi-characters.read_notifications.v1",
#        "esi-corporations.read_divisions.v1",
#        "esi-corporations.read_contacts.v1",
#"esi-assets.read_corporation_assets.v1",
#"esi-corporations.read_titles.v1",
#"esi-corporations.read_blueprints.v1",
#"esi-bookmarks.read_corporation_bookmarks.v1",
#"esi-contracts.read_corporation_contracts.v1",
#"esi-corporations.read_standings.v1",
#"esi-corporations.read_starbases.v1",
#"esi-industry.read_corporation_jobs.v1",
#"esi-markets.read_corporation_orders.v1",
#"esi-corporations.read_container_logs.v1",
#"esi-industry.read_character_mining.v1",
#"esi-industry.read_corporation_mining.v1",
#"esi-planets.read_customs_offices.v1",
#"esi-corporations.read_facilities.v1",
#"esi-corporations.read_medals.v1",
#"esi-characters.read_titles.v1",
#"esi-alliances.read_contacts.v1",
#"esi-characters.read_fw_stats.v1",
#"esi-corporations.read_fw_stats.v1",
#"esi-corporations.read_outposts.v1",
#"esi-characterstats.read.v1"
#]

scopes = ["esi-characters.read_blueprints.v1", "esi-skills.read_skills.v1", "esi-assets.read_assets.v1", "esi-skills.read_skillqueue.v1", "esi-universe.read_structures.v1", "esi-industry.read_character_jobs.v1"]


def validate_eve_jwt(jwt_token):
    """Validate a JWT token retrieved from the EVE SSO.
    Args:
        jwt_token: A JWT token originating from the EVE SSO
    Returns
        dict: The contents of the validated JWT token if there are no
              validation errors
    """

    jwk_set_url = "https://login.eveonline.com/oauth/jwks"

    res = requests.get(jwk_set_url)
    res.raise_for_status()

    data = res.json()

    jwk_sets = data["keys"]

    jwk_set = next((item for item in jwk_sets if item["alg"] == "RS256"))

    return jwt.decode(
        jwt_token,
        jwk_set,
        algorithms=jwk_set["alg"],
        issuer="login.eveonline.com"
    )

class MyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.server.code = self.path.split('?')[1].split('&')[0].split('=')[1]

        self.send_response(200)
        self.end_headers()
        #self.wfile.write('<html><script>document.write(location.hash.substr(1).split("&").map(v => v.split("="))[0][1]);</script></html>')
        self.wfile.write(b'')


class MyServer(HTTPServer):
    def __init__(self, client_address):
        super(MyServer, self).__init__(client_address, MyHandler)
        self.code = None


def new_refresh_token(scope=" ".join(scopes)):
    httpd = MyServer(('localhost', 8080))


    random = base64.urlsafe_b64encode(secrets.token_bytes(32))
    m = hashlib.sha256()
    m.update(random)
    d = m.digest()
    code_challenge = base64.urlsafe_b64encode(d).decode().replace("=", "")

    base_auth_url = "https://login.eveonline.com/v2/oauth/authorize/"
    params = {
        "response_type": "code",
        "redirect_uri": "http://localhost:8080/callback",
        "client_id": "6207584877514be8b23ab78a6163bb82",
        "scope": scope,
        "state": "foo",
        "code_challenge": code_challenge,
        "code_challenge_method": "S256"
    }

    string_params = urllib.parse.urlencode(params)
    full_auth_url = "{}?{}".format(base_auth_url, string_params)
    print(full_auth_url)

    httpd.handle_request()

    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Host": "login.eveonline.com",
    }

    code_verifier = base64.urlsafe_b64encode(random).decode().replace("=", "")
    form_values = {
        "grant_type": "authorization_code",
        "client_id": "6207584877514be8b23ab78a6163bb82",
        "code": httpd.code,
        "code_verifier": code_verifier
    }

    res = requests.post(
        "https://login.eveonline.com/v2/oauth/token",
        data=form_values,
        headers=headers,
    )

    print("Request sent to URL {} with headers {} and form values: "
          "{}\n".format(res.url, headers, form_values))
    res.raise_for_status()
    data = res.json()
    print(data)
    validate_eve_jwt(data["access_token"])


def refresh(scope=" ".join(scopes)):
    refresh_token = open('refresh_token.txt').readline().rstrip('\n')
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Host": "login.eveonline.com",
    }

    form_values = {
        "grant_type": "refresh_token",
        "refresh_token": refresh_token,
        "client_id": "6207584877514be8b23ab78a6163bb82",
        "scope": scope
    }

    res = requests.post(
        "https://login.eveonline.com/v2/oauth/token",
        data=form_values,
        headers=headers,
    )

    print("Request sent to URL {} with headers {} and form values: "
          "{}\n".format(res.url, headers, form_values), file=sys.stderr)
    res.raise_for_status()
    data = res.json()
    print(data, file=sys.stderr)
    jwt = validate_eve_jwt(data["access_token"])
    print(jwt, file=sys.stderr)
    open('refresh_token.txt', 'w').write(data['refresh_token'] + '\n')
    return data["access_token"], jwt["sub"].split(":")[2]


def call_esi(path, access_token):
    headers = {
            "Authorization": "Bearer {}".format(access_token)
            }
    res = requests.get(path, headers=headers)
    print("\nMade request to {} with headers: "
          "{}".format(path, res.request.headers), file=sys.stderr)
    res.raise_for_status()

    return res.json()


def run_json(resource_url):
    access_token, character_id = refresh()
    return call_esi(resource_url.format(character_id=character_id), access_token)


def run_string(resource_url):
    return json.dumps(run_json(resource_url))


def run_stdout(resource_url):
    print(run_string(resource_url))


def main():
    run_stdout(sys.argv[1])


if __name__ == "__main__":
    main()

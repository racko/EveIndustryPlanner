#! /usr/bin/env python
from datetime import datetime, timezone
from jq import jq
from sso import run_json


def industry_jobs():
    return run_json('https://esi.evetech.net/latest/characters/{character_id}/industry/jobs')


def end_times():
    for job in industry_jobs():
        yield datetime.strptime(job['end_date'], '%Y-%m-%dT%H:%M:%S%z')


def ready_jobs():
    for t in end_times():
        if t <= datetime.now(timezone.utc):
            yield t


def main():
    print(list(ready_jobs()))


if __name__ == "__main__":
    main()

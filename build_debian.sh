#!/bin/bash
dpkg-buildpackage -us -uc -nc
dpkg-deb --build debian/postgresql-16-postxml .
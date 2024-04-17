# PostXML
### A postgres extension for manipulating XML values

## Install
```sh
apt install libxslt1-dev libxslt1.1 libxml2 libxml2-dev postgresql-16 postgresql-server-dev-16
pip install --user pstk
touch .env # Add .., ..
pstk --extension --schema-path=postxml--1.0.sql . install
```

## Test
```sh
pstk --extension --schema-path=postxml--1.0.sql . test --replace
```
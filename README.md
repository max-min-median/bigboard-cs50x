# Installing Dependencies
Create a **.env** similar to the **.env.EXAMPLE** file in the repository
## Postgres
```bash
# Installing via apt will automatically create a postgres user
sudo apt install postgresql postgresql-contrib
# Enter postgres shell as user postgres
sudo -u postgres psql
```

In postgres shell:
```sql
CREATE USER bigboard WITH PASSWORD 'mypassword';
CREATE DATABASE bigboard_db OWNER bigboard;
```

Exit postgres shell with **\q**. \
Edit **.env** file's **DATABASE_URL** key to match your user, password, and database name.
For the above example, it would be:
```
DATABASE_URL=postgresql://bigboard:mypassword@localhost:5432/bigboard_db
```

## Docker
```bash
sudo apt install docker.io
docker build -t bigboard-sandbox .
```

## CS50x Speller Distribution Code
```bash
wget https://cdn.cs50.net/2026/x/psets/5/speller.zip
unzip speller.zip
rm speller.zip
```
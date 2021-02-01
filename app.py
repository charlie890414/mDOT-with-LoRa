from flask import Flask
from flask import render_template
from datetime import datetime

import sqlite3

app = Flask(__name__,
            static_url_path='', 
            static_folder='static',
            template_folder='templates')

@app.route("/")
def home():
    conn = sqlite3.connect('example.db')
    c = conn.cursor()
    c.execute("SELECT * FROM beds")
    results = c.fetchall()
    conn.close()

    rows = []

    for row in results:
        rows.append((datetime.utcfromtimestamp(int(row[0])).strftime('%Y-%m-%d %H:%M:%S'), row[1], row[2]))    

    print(rows)
    return render_template("index.html", rows=rows) 

app.run()
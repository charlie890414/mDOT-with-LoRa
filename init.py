import os
import sqlite3

if os.path.exists('example.db'):
    os.remove('example.db')

conn = sqlite3.connect('example.db')

c = conn.cursor()

c.execute('''CREATE TABLE beds
             (time text, status text, device text)''')

# Insert a row of data
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', '008000000000f30f')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")
c.execute("INSERT INTO beds VALUES ('1610117674','working', 'xxx')")

# Save (commit) the changes
conn.commit()

# We can also close the connection if we are done with it.
# Just be sure any changes have been committed or they will be lost.
conn.close()
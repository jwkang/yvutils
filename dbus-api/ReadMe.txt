Execution environment
author : jwkang2
date : 2017-09-27

python >= 3.5

To install python3.5, refer to https://www.python.org/downloads/release/python-354/
$ wget https://www.python.org/ftp/python/3.5.4/Python-3.5.4.tgz
$ tar -zxvf Python-3.5.4.tgz
$ cd Python-3.5.4.tgz
$ ./configure
$ make
# sudo make install
Then, modify python verison via /usr/local/bin (user) and /usr/bin/ (root)
<Best way to modify>
/usr/bin/python -> /usr/local/bin/python -> /usr/local/bin/python3.5

pip >= 9.0.1
To install pip, refer to https://pip.pypa.io/en/stable/installing/
$ wget https://bootstrap.pypa.io/get-pip.py
# sudo python get-pip.py
(It will be installed corresponding to the version of python system now has)

Install packages,
pip install -r requirements.txt


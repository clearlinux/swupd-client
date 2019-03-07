Notes on manual pages
=====================

The manual pages are generated using ``rst2man``\(1). To recreate
them, run ``make man`` in the toplevel folder. If you want to edit the
documentation for upstream changes, make sure to edit the ``*.rst``
files and not the shipped nroff output files.

Notes on debugging Travis failures
==================================

This is based on  https://docs.travis-ci.com/user/common-build-problems/

Travis works by getting a webhook notification when a commit is pushed
to github. It then starts up a docker container, does a shallow git
pull of the repo and the branch, and runs the instructions in the
`.travis.yml`_ file.

.. _.travis.yml: ../.travis.yml

If you run docker locally you can connect into the environment and poke around. So the steps are.

Install docker
--------------
    sudo -E swupd bundle-add containers-basic

Get docker working
------------------
Get `docker run hello-world` working.
TODO: fill in some details - assistance welcome.

Start the travis image
----------------------

At the time of writing,
the current image for the `default` target is
`travisci/ci-garnet:packer-1512502276-986baf0`

Start a detached image running /sbin/init (i.e. a system that is
booted normally). You probably will want this to be privileged, so you can use tools like gdb or strace.

   docker run --name travis-debug -dit --privileged travisci/ci-garnet:packer-1512502276-986baf0 /sbin/init

This should
return pretty quickly. Now connect to it and run a shell.

    docker exec -it travis-debug bash -i

Become the travis user, set up any environment variables needed. These could be added into `~/.bashrc` if desired.

    su - travis
    export http_proxy=... https_proxy=

Pull the code from github
    git clone --depth=50 --branch=master https://github.com/clearlinux/swupd-client.git

Edit the .git/config file to add in the non-standard names that github uses, so it looks like this
::
    [remote "origin"]
        url = https://github.com/clearlinux/swupd-client.git
        fetch = +refs/heads/master:refs/remotes/origin/master
        fetch = +refs/pull/*/head:refs/remotes/origin/pr/*

then you can get it by name, e.g.

    git checkout origin/pr/502

Update the environment according to the commands in the `.travis.yml`_
file, e.g. follow the instructions in the install section of the
file. You can use this crude function to get a section out of a yaml
file.

    S(){ sed '1,/^'"${1?Missing section name}"':/d;/^[^ \t]*:/Q;s/^[ \t]*-//' "${2:-.travis.yml}" ; }

which you can then pipe to a shell after you have checked it, i.e. ::

   S install
           wget https://github.com/libcheck/check/releases/download/0.11.0/check-0.11.0.tar.gz
           tar -xvf check-0.11.0.tar.gz
           pushd check-0.11.0 && ./configure --prefix=/usr && make -j48 && sudo make install && popd
           wget https://github.com/clearlinux/bsdiff/releases/download/v1.0.2/bsdiff-1.0.2.tar.xz
           tar -xvf bsdiff-1.0.2.tar.xz
           pushd bsdiff-1.0.2 && ./configure --prefix=/usr --disable-tests && make -j48 && sudo make install && popd
           wget https://curl.haxx.se/download/curl-7.51.0.tar.gz
           tar -xvf curl-7.51.0.tar.gz
           pushd curl-7.51.0 && ./configure --prefix=/usr && make -j48 && sudo make install && popd
           wget https://download.clearlinux.org/releases/13010/clear/Swupd_Root.pem
           sudo install -D -m0644 Swupd_Root.pem /usr/share/clear/update-ca/Swupd_Root.pem
           wget https://github.com/libarchive/libarchive/archive/v3.3.1.tar.gz
           tar -xvf v3.3.1.tar.gz
           pushd libarchive-3.3.1 && autoreconf -fi && ./configure --prefix=/usr && make && sudo make install && popd
           sudo apt-get install python3-docutils
           sudo apt-get install realpath
           sudo ln -s /usr/share/docutils/scripts/python3/rst2man /usr/bin/rst2man.py
          
          # Ubuntu's default umask is 0002, but this break's swupd hash calculations.
    
    S install | bash -x

Once this is done, you can actually run the job, this is the script section of the `.travis.yml`_ file ::

       S script
          sudo find test/functional -exec chmod g-w {} \;
          autoreconf --verbose --warnings=none --install --force &&
          ./configure &&
          make -j48 &&
          sudo sh -c 'umask 0022 && make -j48 check' &&
          sudo sh -c 'umask 0022 && make install'
   

#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Syntax: $0 <version>"
  exit 1
fi

VERSION=$1

git tag | grep -q "^${VERSION}$"
TAG_OK=$?
 
if [ ${TAG_OK} -ne 0 ]; then
  echo "Tag not found: $VERSION"
  exit 1
fi

sed -e 's/XXVERSIONXX/'${VERSION}'/g' Dockerfile > .dockerfile-${VERSION}
sed -e 's/XXVERSIONXX/'${VERSION}'/g' docker-base-setup.sh > .docker-base-setup-${VERSION}.sh
sed -e 's/XXVERSIONXX/'${VERSION}'/g' Dockerfile-multiconvert > .dockerfile-multiconvert-${VERSION}
sed -e 's/XXVERSIONXX/'${VERSION}'/g' Dockerfile-tools > .dockerfile-tools-${VERSION}

chmod 755 .docker-base-setup-${VERSION}.sh

docker images --format '{{.Repository}}:{{.Tag}}'|grep prevent|sed -e 's/:<none>//g'|xargs docker rmi

docker build -t ry99/prevent --file .dockerfile-${VERSION} .
docker build -t ry99/prevent-cnv --file .dockerfile-multiconvert-${VERSION} .
docker build -t ry99/prevent-tools --file .dockerfile-tools-${VERSION} .

IMAGEID=$(docker image ls|grep 'prevent ' | awk '{print $3}')
echo "prevent image: ${IMAGEID}"
docker tag $IMAGEID "ry99/prevent:latest" 
docker tag $IMAGEID "ry99/prevent:${VERSION}"

IMAGEID=$(docker image ls|grep 'prevent-cnv' | awk '{print $3}')
echo "prevent-cnv image: ${IMAGEID}"
docker tag $IMAGEID "ry99/prevent-cnv:latest"
docker tag $IMAGEID "ry99/prevent-cnv:${VERSION}"

IMAGEID=$(docker image ls|grep 'prevent-tool' | awk '{print $3}')
echo "prevent-tools image: ${IMAGEID}"
docker tag $IMAGEID "ry99/prevent-tools:latest"
docker tag $IMAGEID "ry99/prevent-tools:${VERSION}"

echo
echo 
echo "Now run these:"
echo "docker push ry99/prevent:latest"
echo "docker push ry99/prevent:${VERSION}"
echo "docker push ry99/prevent-cnv:latest"
echo "docker push ry99/prevent-cnv:${VERSION}"
echo "docker push ry99/prevent-tools:latest"
echo "docker push ry99/prevent-tools:${VERSION}"




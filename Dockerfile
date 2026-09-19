# Compile against the published MySQL 8.4 plugin ABI.
ARG MYSQL_RUNTIME_IMAGE=mysql:8.4.8
FROM mysql:8.4.8 AS build
USER root
RUN microdnf install -y gcc gcc-c++ cmake make git libcurl-devel cpio && microdnf clean all
RUN curl -fSL --retry 3 "https://repo.mysql.com/yum/mysql-8.4-community/el/9/$(uname -m)/mysql-community-devel-8.4.8-1.el9.$(uname -m).rpm" -o /tmp/mysql-devel.rpm \
    && cd / && rpm2cpio /tmp/mysql-devel.rpm | cpio -idm \
    && rm /tmp/mysql-devel.rpm
RUN git clone --depth 1 --branch mysql-8.4.8 --filter=blob:none --sparse https://github.com/mysql/mysql-server.git /opt/mysql-source \
    && cd /opt/mysql-source && git sparse-checkout set include
RUN curl -fSL --retry 3 https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp -o /tmp/json.hpp \
    && echo 'aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63  /tmp/json.hpp' | sha256sum -c - \
    && mkdir -p /usr/local/include/nlohmann && mv /tmp/json.hpp /usr/local/include/nlohmann/json.hpp
WORKDIR /src
COPY CMakeLists.txt ./
COPY src ./src
COPY tests ./tests
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMYSQL_SOURCE_INCLUDE_DIR=/opt/mysql-source/include \
    && cmake --build build -j2 && ctest --test-dir build --output-on-failure

# Export this stage to get a plugin bundle for an existing MySQL server.
FROM scratch AS plugin
COPY --from=build /src/build/ailike_udf.so /src/build/ailike_rewrite.so /
COPY sql/install.sql sql/uninstall.sql LICENSE THIRD_PARTY_NOTICES.md /
COPY docs/install.md /INSTALL.md

# Convenience server for the isolated demo and tests.
FROM ${MYSQL_RUNTIME_IMAGE} AS runtime
COPY --from=build /src/build/ailike_udf.so /usr/lib64/mysql/plugin/
COPY --from=build /src/build/ailike_rewrite.so /usr/lib64/mysql/plugin/
COPY sql/install.sql /docker-entrypoint-initdb.d/10-ailike.sql

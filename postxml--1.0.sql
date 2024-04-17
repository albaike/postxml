create schema postxml;

create function postxml.read_text(text)
returns text
as 'postxml','read_text'
language C strict;

create function postxml.xsl_transform(xml,xml)
returns xml
as 'postxml','xsl_transform'
language C strict;

create function postxml.xsd_validate(xml,xml)
returns boolean
as 'postxml','xsd_validate'
language C strict;

create function postxml.xml_docs_equal(xml,xml)
returns boolean
as 'postxml','xml_docs_equal'
language C strict;

create table postxml.xsd_schemas (
    name text not null primary key,
    xsd xml not null
        constraint is_doc check (xsd is document)
);

create function postxml.xsd_validate_name(schema_name text, doc xml)
returns boolean as $$
declare
    xsd_doc xml;
begin
    select xsd into xsd_doc
    from postxml.xsd_schemas
    where name = schema_name;

    return postxml.xsd_validate(xsd_doc, doc);
end;
$$ language plpgsql;

create table postxml.xsl_transforms (
    name text not null primary key,
    transform xml not null
        constraint is_doc check (transform is document)
        constraint is_xslt check (postxml.xsd_validate_name('xslt1.0', transform))
);

create function postxml.xsl_transform_name(transform_name text, doc xml)
returns xml as $$
declare
    xsl_doc xml;
begin
    select transform into xsl_doc
    from postxml.xsl_transforms
    where name = transform_name;

    return postxml.xsl_transform(xsl_doc, doc);
end;
$$ language plpgsql;

create function postxml.init_xsd(xsd_path text)
returns void as $$
begin
    insert into postxml.xsd_schemas values (
        'xsd1.0',
        postxml.read_text(xsd_path)::xml
    );
    alter table postxml.xsd_schemas
    add constraint is_xsd check (postxml.xsd_validate_name('xsd1.0', xsd));
    return;
end;
$$ language plpgsql;
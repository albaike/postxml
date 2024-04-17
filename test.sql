create schema postxml_test;

create function postxml_test.test_xsd()
returns void as $$
begin
    if not postxml.xsd_validate(
        postxml.read_text('/tmp/XMLSchema-10.xsd')::xml,
        postxml.read_text('/tmp/XMLSchema-10.xsd')::xml
    ) then
        raise exception 'XMLSchema 1.0 not valid against itself';
    end if;

    if postxml.xsd_validate(
        postxml.read_text('/tmp/XMLSchema-10.xsd')::xml,
        postxml.read_text('/tmp/fail.xsd')::xml
    ) then
        raise exception 'XSD validated invalid XML Schema.';
    end if;

    perform postxml_test.test_xsd_fail();
    return;
end;
$$ language plpgsql;


create function postxml_test.test_xsl_fail()
returns void as $$
begin
    perform postxml.xsl_transform(
        postxml.read_text('/tmp/fail.xsl')::xml,
        postxml.read_text('/tmp/fail.xsl')::xml
    );

    raise exception 'XSLT failure not raised.';
exception
    when others then return;
end;
$$ language plpgsql;

create function postxml_test.test_xsl()
returns void as $$
declare
    xslt_10_ex_d1 xml;
    xslt_10_ex_d2_html xml;
    xslt_10_ex_d2_svg xml;
begin
    select postxml.xsl_transform(
        postxml.read_text('/tmp/xslt-10-ex-d1.xsl')::xml,
        postxml.read_text('/tmp/xslt-10-ex-d1.xml')::xml
    ) into xslt_10_ex_d1;
    if not postxml.xml_docs_equal(
        xslt_10_ex_d1,
        postxml.read_text('/tmp/xslt-10-ex-d1.xhtml')::xml
    ) then
        raise exception 'Invalid transform for XSLT1.0 Example D.1: %', xslt_10_ex_d1;
    end if;

    select postxml.xsl_transform(
        postxml.read_text('/tmp/xslt-10-ex-d2-html.xsl')::xml,
        postxml.read_text('/tmp/xslt-10-ex-d2-in.xml')::xml
    ) into xslt_10_ex_d2_html;
    if not postxml.xml_docs_equal(
        xslt_10_ex_d2_html,
        postxml.read_text('/tmp/xslt-10-ex-d2-html.xml')::xml
    ) then
        raise exception 'Invalid transform for XSLT1.0 Example D.2: %', xslt_10_ex_d2_html;
    end if;

    select postxml.xsl_transform(
        postxml.read_text('/tmp/xslt-10-ex-d2-svg.xsl')::xml,
        postxml.read_text('/tmp/xslt-10-ex-d2-in.xml')::xml
    ) into xslt_10_ex_d2_svg;
    if not postxml.xml_docs_equal(
        xslt_10_ex_d2_svg,
        postxml.read_text('/tmp/xslt-10-ex-d2-svg.xml')::xml
    ) then
        raise exception 'Invalid transform for XSLT1.0 Example D.2: %', xslt_10_ex_d2_svg;
    end if;

    perform postxml_test.test_xsl_fail();
    return;
end;
$$ language plpgsql;

create function postxml_test.test_xml_docs_equal()
returns void as $$
begin
    if not postxml.xml_docs_equal(
        postxml.read_text('/tmp/xslt-10-ex-d1.xsl')::xml,
        postxml.read_text('/tmp/xslt-10-ex-d1.xsl')::xml
    ) then
        raise exception 'XML docs equal not working';
    end if;

    return;
end;
$$ language plpgsql;

select postxml_test.test_xml_docs_equal();
select postxml.init_xsd('/tmp/XMLSchema-10.xsd');
select postxml_test.test_xsd();
select postxml_test.test_xsl();

drop schema postxml_test cascade;